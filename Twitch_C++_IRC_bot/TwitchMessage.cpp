#include "stdafx.h"
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#include "TwitchMessage.h"
#include "IRC_Bot.h"
#include <boost\algorithm\string\classification.hpp>
#include <boost\algorithm\string\split.hpp>
#include <boost\algorithm\string\replace.hpp>
#include <iostream>
#include <exception>
#include <string_view>
#include <sstream>

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace { // helpers
	using Twitch::irc::message::cap::tags::Badge;
	using Twitch::irc::message::cap::tags::UserType;
	using Twitch::irc::message::cap::tags::BadgeLevel;

	inline Badge string_to_badge(std::string_view badge) {
		if      (badge == "subscriber"sv)  { return Badge::subscriber; }
		else if (badge == "bits"sv)        { return Badge::bits; }
		else if (badge == "turbo"sv)       { return Badge::turbo; }
		else if (badge == "moderator"sv)   { return Badge::moderator; }
		else if (badge == "broadcaster"sv) { return Badge::broadcaster; }
		else if (badge == "global_mod"sv)  { return Badge::global_mod; }
		else if (badge == "admin"sv)       { return Badge::admin; }
		else if (badge == "staff"sv)       { return Badge::staff; }
		else 
			return Badge::unhandled_badge;
	}

	inline std::string badge_to_string(Badge badge) {
		if      (badge == Badge::subscriber)  { return "subscriber"s; }
		else if (badge == Badge::bits)        { return "bits"s; }
		else if (badge == Badge::turbo)       { return "turbo"s; }
		else if (badge == Badge::moderator)   { return "moderator"s; }
		else if (badge == Badge::broadcaster) { return "broadcaster"s; }
		else if (badge == Badge::global_mod)  { return "global_mod"s; }
		else if (badge == Badge::admin)       { return "admin"s; }
		else if (badge == Badge::staff)       { return "staff"s; }
		else
			return "unhandled_badge"s;
	}

	inline std::string user_type_to_string(UserType utype) {
		if      (utype == UserType::empty)      { return ""s; }
		else if (utype == UserType::mod)        { return "mod"s; }
		else if (utype == UserType::global_mod) { return "global_mod"s; }
		else if (utype == UserType::admin)      { return "admin"s; }
		else if (utype == UserType::staff)      { return "staff"s; }
		else
			return "unhandled_type"s;
	}

	std::map<Badge, BadgeLevel> get_badges(std::string_view raw_badges) {
		if (raw_badges.size() == 0) { return {}; }

		std::vector<std::string> splitted;
		boost::split(
			splitted,
			raw_badges,
			boost::is_any_of(",/"),
			boost::algorithm::token_compress_on
		);
		if (splitted.size() % 2 != 0) return {};

		std::map<Badge, BadgeLevel> badges;
		for (std::size_t i{ 0 }; i < splitted.size() / 2; i += 2) {
			badges.emplace(
				string_to_badge(splitted[i]),
				std::stoi(splitted[i + 1])
			);
		}
		return badges;
	}

	inline bool get_mod(std::string_view raw_message) noexcept {
		return raw_message == "1"sv;
	}

	inline bool get_subscriber(std::string_view raw_message) noexcept {
		return raw_message == "1"sv;
	}

	std::time_t get_tmi_sent_ts(std::string_view raw_message) {
		std::stringstream stream{ raw_message.data() };
		std::time_t ts;
		stream >> ts;
		return ts;
	}

	inline bool get_turbo(std::string_view raw_message) noexcept {
		return raw_message == "1"sv;
	}

	inline UserType get_user_type(std::string_view raw_message) {
		// most common case
		if (raw_message.size() == 0) { return UserType::empty; }

		// reverse order as there are more mods than staff members
		else if (raw_message == "mod"sv)        { return UserType::mod; }
		else if (raw_message == "global_mod"sv) { return UserType::global_mod; }
		else if (raw_message == "admin"sv)      { return UserType::admin; }
		else if (raw_message == "staff"sv)      { return UserType::staff; }
		else
			return UserType::unhandled_type;
	}

	std::vector<std::string> get_list_of_names(std::string_view raw_list) {
		if (raw_list == "End of /NAMES list"sv) { return {}; }

		std::vector<std::string> names;
		boost::split(
			names,
			raw_list,
			[](auto c) -> bool { return c == ' '; },
			boost::token_compress_on
		);
		return std::move(names);
	}

	unsigned int get_bits(std::string_view raw_bits) {
		std::stringstream stream{ raw_bits.data() };
		unsigned long ts;
		stream >> ts;
		return ts;
	}
}

namespace Twitch::irc::message {
	const std::regex PING::regex{
		R"(PING :(.+)[\r\n|\r|\n]?)"
	};
	std::optional<PING> PING::is(std::string_view raw_message) {
		std::cmatch match;
		if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

		constexpr const size_t host = 1;

		return PING{
			match.str(host)
		};
	}

	const std::regex Plain_message::regex{
		R"(:(.+)!(.+)@(.+) PRIVMSG (#.+) :(.+)[\r\n|\r|\n]?)"
	};
	std::optional<Plain_message> Plain_message::is(std::string_view raw_message) {
		std::cmatch match;
		if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

		constexpr const size_t nick = 1;
		constexpr const size_t user = 2;
		constexpr const size_t host = 3;
		constexpr const size_t target = 4;
		constexpr const size_t message = 5;

		return Plain_message{
			std::move(match.str(nick)),
			std::move(match.str(user)),
			std::move(match.str(host)),
			std::move(match.str(target)),
			std::move(match.str(message))
		};
	}

	namespace cap {
		namespace membership {
			const std::regex JOIN::regex{
				// first 3 groups are identical
				R"(:(.+)!(?:.+)@(?:.+).tmi.twitch.tv JOIN (#.+)[\r\n|\r|\n]?)"
			};
			std::optional<JOIN> JOIN::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t user    = 1;
				constexpr const size_t channel = 2;

				return JOIN{
					match.str(user),
					match.str(channel)
				};
			}

			const std::regex MODE::regex{
				R"(:jtv MODE (#.+) ([+-])o (.+)[\r\n|\r|\n]?)"
			};
			std::optional<MODE> MODE::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t channel = 1;
				constexpr const size_t symbol  = 2;
				constexpr const size_t user    = 3;

				return MODE{
					match.str(channel),
					match.str(symbol)[0] == '+',
					match.str(user)
				};
			}

			const std::string NAMES::EOL = "366"s;
			const std::regex NAMES::regex{
				R"(:(.+).tmi.twitch.tv (353|366) (?:.+) (?:= )?(#.+) :(.+)[\r\n|\r|\n]?)"
			};
			std::optional<NAMES> NAMES::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t user   = 1;
				constexpr const size_t msg_id = 2;
				constexpr const size_t channel = 3;
				constexpr const size_t list   = 4;

				return NAMES{
					match.str(user),
					match.str(msg_id) == NAMES::EOL,
					match.str(channel),
					get_list_of_names(match.str(list)),
				};
			}

			const std::regex PART::regex{
				// first 3 groups are identical
				R"(:(.+)!(?:.+)@(?:.+).tmi.twitch.tv PART (#.+)[\r\n|\r|\n]?)"
			};
			std::optional<PART> PART::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t user = 1;
				constexpr const size_t channel = 2;

				return PART{
					match.str(user),
					match.str(channel)
				};
			}

		}
		namespace tags {
			const std::regex CLEARCHAT::regex{
				R"(@(?:ban-duration=([0-9]+);)?ban-reason=(.*) :tmi.twitch.tv CLEARCHAT (#.+) :(.+)[\r\n|\r|\n]?)"
			};
			std::optional<CLEARCHAT> CLEARCHAT::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t duration = 1;
				constexpr const size_t reason = 2;
				constexpr const size_t channel = 3;
				constexpr const size_t user = 4;

				return CLEARCHAT{
					std::atoi(match.str(duration).c_str()),
					boost::replace_all_copy(match.str(reason), R"(\s)", " "),
					match.str(channel),
					match.str(user)
				};
			}

			CLEARCHAT::CLEARCHAT(
				std::time_t&& t_duration,
				std::string&& t_reason,
				std::string&& t_channel,
				std::string&& t_user
			) :
				cap::commands::CLEARCHAT{ std::move(t_channel), std::move(t_user) },
				ban_duration(std::move(t_duration)),
				ban_reason(std::move(t_reason))
			{
			}

			const std::regex GLOBALUSERSTATE::regex{
				R"(@color=([^;]*);display-name=([^;]*);emote-sets=([^;]*);turbo=([01]);)"
				R"(user-id=([^;]+);user-type=([^ ]*) :tmi.twitch.tv GLOBALUSERSTATE[\r\n|\r|\n]?)"
			};
			std::optional<GLOBALUSERSTATE> GLOBALUSERSTATE::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t color = 1;
				constexpr const size_t display_name = 2;
				constexpr const size_t emote_sets = 3;
				constexpr const size_t turbo = 4;
				constexpr const size_t user_id = 5;
				constexpr const size_t user_type = 6;

				return GLOBALUSERSTATE{
					match.str(color),
					match.str(display_name),
					match.str(emote_sets),
					match.str(turbo)[0] == '1',
					match.str(user_id),
					get_user_type(match.str(user_type))
				};
			}

			const std::regex PRIVMSG::regex{
				R"(@badges=([^;]*);(?:bits=([0-9]+);)?color=([^;]*);display-name=([^;]*);emotes=([^;]*);id=([^;]+);mod=([01]);room-id=([0-9]+);)"
				R"(subscriber=([01]);tmi-sent-ts=([0-9]+);turbo=([01]);user-id=([0-9]+);user-type=([^ ]*))"
				R"( :(.+)!(.+)@(.+) PRIVMSG (#.+) :(.+)[\r\n|\r|\n]?)"
			};
			std::optional<PRIVMSG> PRIVMSG::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t badges = 1;
				constexpr const size_t bits = 2;
				constexpr const size_t color = 3;
				constexpr const size_t display_name = 4;
				constexpr const size_t emotes = 5;
				constexpr const size_t id = 6;
				constexpr const size_t mod = 7;
				constexpr const size_t room_id = 8;
				constexpr const size_t subscriber = 9;
				constexpr const size_t tmi_sent_ts = 10;
				constexpr const size_t turbo = 11;
				constexpr const size_t user_id = 12;
				constexpr const size_t user_type = 13;
				constexpr const size_t nick = 14;
				constexpr const size_t user = 15;
				constexpr const size_t host = 16;
				constexpr const size_t target = 17;
				constexpr const size_t message = 18;

				return PRIVMSG{
					Plain_message{
						std::move(match.str(nick)),
						std::move(match.str(user)),
						std::move(match.str(host)),
						std::move(match.str(target)),
						std::move(match.str(message))
					},
					get_badges(match.str(badges)),
					get_bits(match.str(bits)),
					match.str(color),
					match.str(display_name),
					match.str(emotes),
					match.str(id),
					get_mod(match.str(mod)),
					match.str(room_id),
					get_subscriber(match.str(subscriber)),
					get_tmi_sent_ts(match.str(tmi_sent_ts)),
					get_turbo(match.str(turbo)),
					match.str(user_id),
					get_user_type(match.str(user_type))
				};
			}

			inline bool PRIVMSG::is_bitsmsg() const noexcept {
				return bits != 0;
			}

			PRIVMSG::PRIVMSG(
				message::Plain_message&&      t_plain,
				std::map<Badge, BadgeLevel>&& t_badge,
				unsigned int  t_bits,
				std::string&& t_color,
				std::string&& t_display_name,
				std::string&& t_emotes,
				std::string&& t_id,
				bool          t_mod,
				std::string&& t_room_id,
				bool          t_subscriber,
				std::time_t&& t_tmi_sent_ts,
				bool          t_turbo,
				std::string&& t_user_id,
				UserType      t_user_type
			) :
				message::Plain_message{ std::move(t_plain) },
				bits(t_bits),
				badges(std::move(t_badge)),
				color(std::move(t_color)),
				display_name(std::move(t_display_name)),
				emotes(std::move(t_emotes)),
				id(std::move(t_id)),
				mod(t_mod),
				room_id(std::move(t_room_id)),
				subscriber(t_subscriber),
				tmi_sent_ts(std::move(t_tmi_sent_ts)),
				turbo(t_turbo),
				user_id(std::move(t_user_id)),
				user_type(std::move(t_user_type))
			{
			}

			const std::regex ROOMSTATE::regex{
				R"(@(?:broadcaster-lang=([^;]*);?)?(?:r9k=([01]);?)?(?:slow=([0-9]+);?)?)"
				R"((?:subs-only=([01]))? :tmi.twitch.tv ROOMSTATE (#.+)[\r\n|\r|\n]?)"
			};
			std::optional<ROOMSTATE> ROOMSTATE::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t broadcaster_lang = 1;
				constexpr const size_t r9k = 2;
				constexpr const size_t slow = 3;
				constexpr const size_t subs_only = 4;
				constexpr const size_t channel = 5;

				const auto get_lang = [&](const std::string str) -> std::optional<const std::string>
				{
					if (str.size() == 0)
						return std::nullopt;

					return str;
				};
				const auto get_flag = [&](const std::string str) -> std::optional<const bool>
				{
					if (str.size() == 0)
						return std::nullopt;

					return str[0] == '1';
				};
				const auto get_slow = [&](const std::string str) -> std::optional<const std::time_t>
				{
					if (str.size() == 0)
						return std::nullopt;

					std::stringstream stream;
					std::time_t time{};
					stream >> time;
					return time;
				};

				return ROOMSTATE{
					cap::commands::ROOMSTATE{ match.str(channel) },
					get_lang(match.str(broadcaster_lang)),
					get_flag(match.str(r9k)),
					get_slow(match.str(slow)),
					get_flag(match.str(subs_only))
				};
			}

			ROOMSTATE::ROOMSTATE(
				cap::commands::ROOMSTATE&& t_roomstate,
				std::optional<const std::string>&& t_lang,
				std::optional<const bool>&&        t_r9k,
				std::optional<const std::time_t>&& t_slow,
				std::optional<const bool>&&        t_subs_only
			) :
				cap::commands::ROOMSTATE{ t_roomstate },
				broadcaster_lang(std::move(t_lang)),
				r9k(std::move(t_r9k)),
				slow(std::move(t_slow)),
				subs_only(std::move(t_subs_only))
			{
			}

			const std::regex USERNOTICE::Sub::regex{
				R"(msg-id=(?:sub|resub);msg-param-months=([0-9]+);msg-param-sub-plan=(.+);msg-param-sub-plan-name=(.+))"
			};
			std::optional<USERNOTICE::Sub> USERNOTICE::Sub::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const std::size_t msg_param_months = 1;
				constexpr const std::size_t msg_param_sub_plan = 2;
				constexpr const std::size_t msg_param_sub_plan_name = 3;

				return Sub{
					std::atoi(match.str(msg_param_months).c_str()),
					match.str(msg_param_sub_plan),
					boost::replace_all_copy(match.str(msg_param_sub_plan_name), R"(\s)", " ")
				};
			}

			const std::regex USERNOTICE::Subgift::regex{
				R"(msg-id=subgift;msg-param-months=([0-9]+);msg-param-recipient-display-name=(.*);)"
				R"(msg-param-recipient-id=(.+);msg-param-recipient-name=(.+);)"
				R"(msg-param-sub-plan-name=(.*);msg-param-sub-plan=(.+))"
			};
			std::optional<USERNOTICE::Subgift> USERNOTICE::Subgift::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t msg_param_months = 1;
				constexpr const size_t msg_param_recipient_display_name = 2;
				constexpr const size_t msg_param_recipient_id = 3;
				constexpr const size_t msg_param_recipient_name = 4;
				constexpr const size_t msg_param_sub_plan_name = 5;
				constexpr const size_t msg_param_sub_plan = 6;

				return Subgift{
					std::atoi(match.str(msg_param_months).c_str()),
					match.str(msg_param_recipient_display_name),
					match.str(msg_param_recipient_id),
					match.str(msg_param_recipient_name),
					boost::replace_all_copy(match.str(msg_param_sub_plan_name), R"(\s)", " "),
					match.str(msg_param_sub_plan),
				};
			}

			const std::regex USERNOTICE::Raid::regex{
				R"(msg-id=raid;msg-param-displayName=(.*);msg-param-login=(.+);msg-param-viewerCount=([0-9]*))"
			};
			std::optional<USERNOTICE::Raid> USERNOTICE::Raid::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t msg_param_displayName = 1;
				constexpr const size_t msg_param_login = 2;
				constexpr const size_t msg_param_viewerCount = 3;

				return Raid{
					match.str(msg_param_displayName),
					match.str(msg_param_login),
					std::atoi(match.str(msg_param_viewerCount).c_str())
				};
			}

			const std::regex USERNOTICE::Ritual::regex{
				R"(msg-id=ritual;msg-param-ritual-name=new_chatter)" // only one valid value for ritual name
			};
			std::optional<USERNOTICE::Ritual> USERNOTICE::Ritual::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				return Ritual{};
			}

			const std::regex USERNOTICE::regex{
				R"(@badges=(.*);color=(.*);display-name=(.*);emotes=(.*);)"
				R"(id=(.+);login=(.+);mod=([01]);(msg-id=.+);room-id=(.+);)"
				R"(subscriber=([01]);system-msg=(.*);tmi-sent-ts=([0-9]+);turbo=([01]);)"
				R"(user-id=(.+);user-type=(.*) :tmi.twitch.tv USERNOTICE (#.+)(?: :(.+))?[\r\n|\r|\n]?)"
			};
			std::optional<USERNOTICE> USERNOTICE::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t badge = 1;
				constexpr const size_t color = 2;
				constexpr const size_t display_name = 3;
				constexpr const size_t emotes = 4;
				constexpr const size_t id = 5;
				constexpr const size_t login = 6;
				constexpr const size_t mod = 7;
				constexpr const size_t msg_id = 8;
				constexpr const size_t room_id = 9;
				constexpr const size_t subscriber = 10;
				constexpr const size_t system_msg = 11;
				constexpr const size_t tmi_sent_ts = 12;
				constexpr const size_t turbo = 13;
				constexpr const size_t user_id = 14;
				constexpr const size_t user_type = 15;
				constexpr const size_t channel = 16;
				constexpr const size_t message = 17;

				auto get_msg_id_details =
					[&]() -> boost::variant<ParseError, Sub, Subgift, Raid, Ritual>
				{
					if (auto parsed = Sub::is(match.str(msg_id)); parsed) {
						return std::move(*parsed);
					}
					else if (auto parsed = Subgift::is(match.str(msg_id)); parsed) {
						return std::move(*parsed);
					}
					else if (auto parsed = Raid::is(match.str(msg_id)); parsed) {
						return std::move(*parsed);
					}
					else if (auto parsed = Ritual::is(match.str(msg_id)); parsed) {
						return std::move(*parsed);
					}
					else return ParseError{};
				};

				return USERNOTICE{
					cap::commands::USERNOTICE{ match.str(channel), match.str(message) },
					get_badges(match.str(badge)),
					match.str(color),
					match.str(display_name),
					match.str(emotes),
					match.str(id),
					match.str(login),
					match.str(mod)[0] == '1',
					get_msg_id_details(),
					match.str(room_id),
					match.str(subscriber)[0] == '1',
					boost::replace_all_copy(match.str(system_msg), R"(\s)", " "),
					get_tmi_sent_ts(match.str(tmi_sent_ts)),
					match.str(turbo)[0] == '1',
					match.str(user_id),
					get_user_type(match.str(user_type))
				};
			}

			USERNOTICE::USERNOTICE(
				cap::commands::USERNOTICE&&   t_usernotice,
				std::map<Badge, BadgeLevel>&& t_badges,
				std::string&& t_color,
				std::string&& t_display_name,
				std::string&& t_emotes,
				std::string&& t_id,
				std::string&& t_login,
				bool          t_mod,
				boost::variant<ParseError, Sub, Subgift, Raid, Ritual>&& t_msg_id,
				std::string&& t_room_id,
				bool          t_subscriber,
				std::string&& t_system_msg,
				std::time_t&& t_tmi_sent_ts,
				bool          t_turbo,
				std::string&& t_user_id,
				UserType      t_user_type
			) :
				cap::commands::USERNOTICE{ std::move(t_usernotice) },
				badges(std::move(t_badges)),
				color(std::move(t_color)),
				display_name(std::move(t_display_name)),
				emotes(std::move(t_emotes)),
				id(std::move(t_id)),
				login(std::move(t_login)),
				mod(t_mod),
				msg_id(std::move(t_msg_id)),
				room_id(std::move(t_room_id)),
				subscriber(t_subscriber),
				system_msg(std::move(t_system_msg)),
				tmi_sent_ts(std::move(t_tmi_sent_ts)),
				turbo(t_turbo),
				user_id(std::move(t_user_id)),
				user_type(t_user_type)
			{
			}

			const std::regex USERSTATE::regex{
				R"(@color=(.*);display-name=(.*);emote-sets=(.*);)"
				R"(mod=([01]);subscriber=([01]);turbo=([01]);user-type=(.*))"
				R"( :tmi.twitch.tv USERSTATE (#.+)[\r\n|\r|\n]?)"
			};
			std::optional<USERSTATE> USERSTATE::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t color = 1;
				constexpr const size_t display_name = 2;
				constexpr const size_t emote_sets = 3;
				constexpr const size_t mod = 4;
				constexpr const size_t subscriber = 5;
				constexpr const size_t turbo = 6;
				constexpr const size_t user_type = 7;
				constexpr const size_t channel = 8;

				return USERSTATE{
					cap::commands::USERSTATE{ match.str(channel) },
					match.str(color),
					match.str(display_name),
					match.str(emote_sets),
					match.str(mod)[0] == '1',
					match.str(subscriber)[0] == '1',
					match.str(turbo)[0] == '1',
					get_user_type(match.str(user_type))
				};
			}

			USERSTATE::USERSTATE(
				cap::commands::USERSTATE&& t_userstate,
				std::string&& t_color,
				std::string&& t_display_name,
				std::string&& t_emote_sets,
				bool          t_mod,
				bool          t_subscriber,
				bool          t_turbo,
				UserType      t_user_type
			) :
				cap::commands::USERSTATE{ std::move(t_userstate) },
				color(std::move(t_color)),
				display_name(std::move(t_display_name)),
				emote_sets(std::move(t_emote_sets)),
				mod(t_mod),
				subscriber(t_subscriber),
				turbo(t_turbo),
				user_type(t_user_type)
			{
			}
		}
		namespace commands {
			const std::regex CLEARCHAT::regex{
				R"(:tmi.twitch.tv CLEARCHAT (#.+)(?: :(.+))?[\r\n|\r|\n]?)"
			};
			std::optional<CLEARCHAT> CLEARCHAT::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const std::size_t channel = 1;
				constexpr const std::size_t user    = 2;

				return CLEARCHAT{
					match.str(channel),
					match.str(user)
				};
			}

			const std::regex HOSTTARGET::regex{
				R"(:tmi.twitch.tv HOSTTARGET (#.+) ([^ ]+|:-) (?:\[([0-9]*)\])?[\r\n|\r|\n]?)"
			};
			std::optional<HOSTTARGET> HOSTTARGET::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const std::size_t hosting_channel = 1;
				constexpr const std::size_t target_channel  = 2;
				constexpr const std::size_t viewers_count   = 3;

				return HOSTTARGET{
					match.str(hosting_channel),
					match.str(target_channel),
					std::atoi(match.str(viewers_count).c_str())
				};
			}

			const std::regex NOTICE::regex{
				R"(@msg-id=(.+):tmi.twitch.tv NOTICE (#.+) :(.+)[\r\n|\r|\n]?)"
			};
			std::optional<NOTICE> NOTICE::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const std::size_t msg_id  = 1;
				constexpr const std::size_t message = 2;

				return NOTICE{
					match.str(msg_id),
					match.str(message),
				};
			}

			const std::regex RECONNECT::regex{
				R"(RECONNECT[\r\n|\r|\n]?)"
			};
			std::optional<RECONNECT> RECONNECT::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				return RECONNECT{};
			}

			const std::regex ROOMSTATE::regex{
				R"(:tmi.twitch.tv ROOMSTATE (#.+)[\r\n|\r|\n]?)"
			};
			std::optional<ROOMSTATE> ROOMSTATE::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const std::size_t channel = 1;

				return ROOMSTATE{
					match.str(channel)
				};
			}

			const std::regex USERNOTICE::regex{
				R"(:tmi.twitch.tv USERNOTICE (#.+) :(.+)[\r\n|\r|\n]?)"
			};
			std::optional<USERNOTICE> USERNOTICE::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const std::size_t channel = 1;
				constexpr const std::size_t message = 2;

				return USERNOTICE{
					match.str(channel),
					match.str(message)
				};
			}

			const std::regex USERSTATE::regex{
				R"(:tmi.twitch.tv USERSTATE (#.+)[\r\n|\r|\n]?)"
			};
			std::optional<USERSTATE> USERSTATE::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const std::size_t channel = 1;

				return USERSTATE{
					match.str(channel)
				};
			}
		}
	}
	namespace MessageParserHelpers {
		using cap::tags::PRIVMSG;
		//using cap::tags::BITSMSG;
		std::optional<PING> is_ping_message(std::string_view raw_message) {
			std::cmatch match;

			static const std::regex ping_regex{
				R"(PING :(.+)[\r\n|\r|\n]?)"
			};
			if (!std::regex_match(raw_message.data(), match, ping_regex)) { return std::nullopt; }

			constexpr const size_t host = 1;

			return PING{
				match.str(host)
			};
		}

		std::optional<PRIVMSG> is_privmsg_message(std::string_view raw_message) {
			std::cmatch match;

			static const std::regex privmsg_regex{
				R"(@badges=([^;]*);color=([^;]*);display-name=([^;]*);emotes=([^;]*);id=([^;]+);mod=([01]);room-id=([0-9]+);)"
				R"(subscriber=([01]);tmi-sent-ts=([0-9]+);turbo=([01]);user-id=([0-9]+);user-type=([^ ]*))"
				R"( :(.+)!(.+)@(.+) PRIVMSG (#.+) :(.+)[\r\n|\r|\n]?)"
			};
			if (!std::regex_match(raw_message.data(), match, privmsg_regex)) { return std::nullopt; }

			constexpr const size_t badges       = 1;
			constexpr const size_t color        = 2;
			constexpr const size_t display_name = 3;
			constexpr const size_t emotes       = 4;
			constexpr const size_t id           = 5;
			constexpr const size_t mod          = 6;
			constexpr const size_t room_id      = 7;
			constexpr const size_t subscriber   = 8;
			constexpr const size_t tmi_sent_ts  = 9;
			constexpr const size_t turbo        = 10;
			constexpr const size_t user_id      = 11;
			constexpr const size_t user_type    = 12;
			constexpr const size_t nick         = 13;
			constexpr const size_t user         = 14;
			constexpr const size_t host         = 15;
			constexpr const size_t target       = 16;
			constexpr const size_t message      = 17;

			return {};/*PRIVMSG{
				PRIVMSG::Tags{
					get_badges(match.str(badges)),
					match.str(color),
					match.str(display_name),
					match.str(emotes),
					match.str(id),
					get_mod(match.str(mod)),
					match.str(room_id),
					get_subscriber(match.str(subscriber)),
					get_tmi_sent_ts(match.str(tmi_sent_ts)),
					get_turbo(match.str(turbo)),
					match.str(user_id),
					get_user_type(match.str(user_type))
				},
				Plain_message{
					std::move(match.str(nick)),
					std::move(match.str(user)),
					std::move(match.str(host)),
					std::move(match.str(target)),
					std::move(match.str(message))
				}
			}*/
		}

		std::optional<Plain_message> is_plain_privmsg_message(std::string_view raw_message) {
			std::cmatch match;

			static const std::regex plain_message_regex{
				R"(:(.+)!(.+)@(.+) PRIVMSG (#.+) :(.+)[\r\n|\r|\n]?)"
			};
			if (!std::regex_match(raw_message.data(), match, plain_message_regex)) { return std::nullopt; }

			constexpr const size_t nick = 1;
			constexpr const size_t user = 2;
			constexpr const size_t host = 3;
			constexpr const size_t target = 4;
			constexpr const size_t message = 5;

			return Plain_message{
				std::move(match.str(nick)),
				std::move(match.str(user)),
				std::move(match.str(host)),
				std::move(match.str(target)),
				std::move(match.str(message))
			};
		}
	}

	MessageParser::result_t MessageParser::process(std::string_view recived_message) {
		using namespace std::string_literals;
		try
		{
			//using MessageParserHelpers::is_bits_message;
			using MessageParserHelpers::is_ping_message;
			using MessageParserHelpers::is_plain_privmsg_message;
			using MessageParserHelpers::is_privmsg_message;

			if (auto parsed = is_ping_message(recived_message); parsed) {
				return *parsed;
			}
			// cap tags
			if (auto parsed = is_privmsg_message(recived_message); parsed) {
				return *parsed;
			}
			/*if (auto parsed = is_bits_message(recived_message); parsed) {
				return *parsed;
			}*/
		}
		catch (const std::regex_error& e) {
			return ParseError{ e.what() };
		}
		catch (const std::exception& e) {
			return ParseError{ e.what() };
		}
		catch (...) {
			return ParseError{ "Unknown exception"s };
		}

		return ParseError{ "Message type not handled: "s + recived_message.data() };
	}

	MessageParser::MessageParser(std::shared_ptr<IRCWriter> irc_writer)
		: m_writer(irc_writer)
	{
	}
}

namespace message    = Twitch::irc::message;
namespace membership = message::cap::membership;
namespace tags       = message::cap::tags;
namespace commands   = message::cap::commands;

std::ostream& operator<<(std::ostream& stream, const tags::Badge& badge) {
	if      (badge == tags::Badge::subscriber)  { return stream << "subscriber"; }
	else if (badge == tags::Badge::bits)        { return stream << "bits"; }
	else if (badge == tags::Badge::turbo)       { return stream << "turbo"; }
	else if (badge == tags::Badge::moderator)   { return stream << "moderator"; }
	else if (badge == tags::Badge::broadcaster) { return stream << "broadcaster"; }
	else if (badge == tags::Badge::global_mod)  { return stream << "global_mod"; }
	else if (badge == tags::Badge::admin)       { return stream << "admin"; }
	else if (badge == tags::Badge::staff)       { return stream << "staff"; }
	else
		return stream << "unhandled_badge";
}

std::ostream& operator<<(std::ostream& stream, const message::PING& ping) {
	return stream << "\nPING :" << ping.host << '\n';
}

std::ostream& operator<<(std::ostream& stream, const message::Plain_message& plainmsg) {
	return stream << ':'
		<< plainmsg.nick << '!'
		<< plainmsg.user << '@'
		<< plainmsg.host
		<< plainmsg.target << " :"
		<< plainmsg.message
		<< '\n';
}

std::ostream& operator<<(std::ostream& stream, const tags::PRIVMSG& privmsg) {
	return stream << "@badges=";
	for (auto const& [badge, level] : privmsg.badges) {
		stream << badge << '/' << level;
	}
	stream << ";color=" << privmsg.color;
	if (privmsg.is_bitsmsg()) {
		stream << ";bits=" << privmsg.bits;
	}
	stream
		<< ";display-name=" << privmsg.display_name
		<< ";emotes="       << privmsg.emotes
		<< ";id="           << privmsg.id
		<< ";mod="          << privmsg.mod
		<< ";room-id="      << privmsg.room_id
		<< ";subscriber="   << privmsg.subscriber
		<< ";tmi-sent-ts="  << privmsg.tmi_sent_ts
		<< ";turbo="        << privmsg.turbo
		<< ";user-id="      << privmsg.user_id
		<< ";user-type="    << user_type_to_string(privmsg.user_type) << ' '
		<< static_cast<message::Plain_message>(privmsg);
}

//std::ostream& operator<<(std::ostream& stream, const tags::BITSMSG& bits) {
//	return stream
//		<< bits.tags << ' '
//		<< bits.plain;
//}

//std::ostream& operator<<(std::ostream& stream, const tags::PRIVMSG::Tags& tags) {
//	stream << "@badges=";
//	for (auto const& [badge, level] : tags.badges) {
//		stream << badge << '/' << level;
//	}
//	return stream
//		<< ";color=" << tags.color
//		<< ";display-name=" << tags.display_name
//		<< ";emotes=" << tags.emotes
//		<< ";id=" << tags.id
//		<< ";mod=" << tags.mod
//		<< ";room-id=" << tags.room_id
//		<< ";subscriber=" << tags.subscriber
//		<< ";tmi-sent-ts=" << tags.tmi_sent_ts
//		<< ";turbo=" << tags.turbo
//		<< ";user-id=" << tags.user_id
//		<< ";user-type=" << user_type_to_string(tags.user_type);
//}

//std::ostream& operator<<(std::ostream& stream, const tags::BITSMSG::Tags& tags) {
//	stream << "@badges=";
//	for (auto const&[badge, level] : tags.badges) {
//		stream << badge << '/' << level;
//	}
//	return stream
//		<< ";bits=" << tags.bits
//		<< ";color=" << tags.color
//		<< ";display-name=" << tags.display_name
//		<< ";emotes=" << tags.emotes
//		<< ";id=" << tags.id
//		<< ";mod=" << tags.mod
//		<< ";room-id=" << tags.room_id
//		<< ";subscriber=" << tags.subscriber
//		<< ";tmi-sent-ts=" << tags.tmi_sent_ts
//		<< ";turbo=" << tags.turbo
//		<< ";user-id=" << tags.user_id
//		<< ";user-type=" << user_type_to_string(tags.user_type);
//}