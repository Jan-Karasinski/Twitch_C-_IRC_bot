#include "stdafx.h"
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#include "TwitchMessage.h"
#include "IRC_Bot.h"
#include <boost\log\trivial.hpp>
#include <boost\algorithm\string\classification.hpp>
#include <boost\algorithm\string\split.hpp>
#include <boost\algorithm\string\replace.hpp>
#include <iostream>
#include <exception>
#include <string_view>
#include <sstream>
#include <type_traits>

namespace { // helpers
	using Twitch::irc::message::cap::tags::Badge;
	using Twitch::irc::message::cap::tags::BadgeLevel;
	using Twitch::irc::message::cap::tags::UserType;

	std::map<Badge, BadgeLevel> get_badges(std::string_view raw_badges) {
		if (raw_badges.empty()) { return {}; }

		std::vector<std::string> splitted;
		boost::split(
			splitted,
			raw_badges,
			boost::is_any_of(",/"),
			boost::algorithm::token_compress_on
		);
		if (splitted.size() % 2 != 0 || splitted.empty()) { return {}; }

		std::map<Badge, BadgeLevel> badges;
		for (std::size_t i{ 0 }; i < splitted.size(); i += 2) {
			badges.emplace(
				Badge::from_string(splitted[i]),
				std::stoi(splitted[i + 1])
			);
		}
		return badges;
	}

	inline bool get_flag(std::string_view raw_message) noexcept {
		using namespace std::string_view_literals;
		return raw_message == "1"sv;
	}

	std::vector<std::string> get_list_of_names(std::string_view raw_list) {
		using namespace std::string_view_literals;
		if (raw_list == "End of /NAMES list"sv) { return {}; }

		std::vector<std::string> names;
		boost::split(
			names,
			raw_list,
			[](auto c) -> bool { return c == ' '; },
			boost::token_compress_on
		);
		return names;
	}

	unsigned int get_bits(std::string_view raw_bits) {
		std::stringstream stream{ raw_bits.data() };
		unsigned long bits{ 0 };
		stream >> bits;
		return bits;
	}

	template<typename T>
	std::optional<T> get_optional(std::string&& raw) {
		try {
			std::stringstream str{ raw };
			T result{};
			str >> result;
			return result;
		}
		catch (...) {
			return std::nullopt;
		}
	}

	template<>
	std::optional<bool> get_optional(std::string&& raw) {
		using namespace std::string_literals;
		if (raw == "1"s) { return 1; }
		if (raw == "0"s) { return 0; }

		return std::nullopt;
	}

	template<>
	std::optional<std::string> get_optional(std::string&& raw) {
		if (raw.empty()) { return std::nullopt; }

		return std::move(raw);
	}

	template<>
	std::optional<Twitch::irc::message::cap::tags::timestamp_t> get_optional(std::string&& raw) {
		try {
			return Twitch::irc::message::cap::tags::timestamp_t{ std::stoll(raw) };
		}
		catch (...) {
			return std::nullopt;
		}
	}

	template<>
	std::optional<int> get_optional(std::string&& raw) {
		try {
			return std::stoi(raw);
		}
		catch (...) {
			return std::nullopt;
		}
	}

	inline Twitch::irc::message::cap::tags::timestamp_t get_ts(std::string&& raw) {
		using namespace std::chrono_literals;
		return get_optional<Twitch::irc::message::cap::tags::timestamp_t>(std::move(raw)).value_or(0s);
	}

	auto get_color(const std::string& raw_color) {
		using Twitch::irc::message::cap::tags::Color;
		// raw_color format = #RRGGBB
		constexpr const std::size_t proper_length = 7;
		if (raw_color.size() != proper_length) { return Color{}; }

		constexpr const std::size_t offset = 1;
		constexpr const std::size_t length = 2;
		constexpr const std::size_t r = offset;
		constexpr const std::size_t g = r + length;
		constexpr const std::size_t b = g + length;

		return Color::from_string(
			raw_color.substr(r, length),
			raw_color.substr(g, length),
			raw_color.substr(b, length)
		);
	};

}

namespace Twitch::irc::message {
	const std::regex PING::regex{
		"PING :(.+)[\r\n|\r|\n]?"
	};
	std::optional<PING> PING::is(std::string_view raw_message) {
		std::cmatch match;
		if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

		constexpr const size_t host = 1;

		return PING{
			match.str(host)
		};
	}

	bool operator==(const PING& lhs, const PING& rhs) {
		return lhs.host == rhs.host;
	}

	bool operator!=(const PING& lhs, const PING& rhs) {
		return !(lhs == rhs);
	}

	const std::regex PRIVMSG::regex{
		":([^!]+)!(?:[^@]+)@(?:[^.]+).([^ ]+) PRIVMSG (#[^ ]+) :(.+)[\r\n|\r|\n]?"
	};
	std::optional<PRIVMSG> PRIVMSG::is(std::string_view raw_message) {
		std::cmatch match;
		if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

		constexpr const size_t user    = 1;
		constexpr const size_t host    = 2;
		constexpr const size_t channel = 3;
		constexpr const size_t message = 4;

		return PRIVMSG{
			match.str(user),
			match.str(host),
			match.str(channel),
			match.str(message)
		};
	}

	bool operator==(const PRIVMSG& lhs, const PRIVMSG& rhs) {
		return lhs.user    == rhs.user
			&& lhs.host    == rhs.host
			&& lhs.channel == rhs.channel
			&& lhs.message == rhs.message;
	}

	bool operator!=(const PRIVMSG& lhs, const PRIVMSG& rhs) {
		return !(lhs == rhs);
	}

	namespace cap {
		namespace membership {
			const std::regex JOIN::regex{
				// first 3 groups are identical
				":(.+)!(?:.+)@(?:.+).tmi.twitch.tv JOIN (#.+)[\r\n|\r|\n]?"
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

			bool operator==(const JOIN& lhs, const JOIN& rhs) {
				return lhs.user    == rhs.user
					&& lhs.channel == rhs.channel;
			}

			bool operator!=(const JOIN& lhs, const JOIN& rhs) {
				return !(lhs == rhs);
			}

			const std::regex MODE::regex{
				":jtv MODE (#.+) ([+-])o (.+)[\r\n|\r|\n]?"
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

			bool operator==(const MODE& lhs, const MODE& rhs) {
				return lhs.channel == rhs.channel
					&& lhs.gained  == rhs.gained
					&& lhs.user    == rhs.user;
			}

			bool operator!=(const MODE& lhs, const MODE& rhs) {
				return !(lhs == rhs);
			}

			const std::regex NAMES::regex{
				":(.+).tmi.twitch.tv (353|366) (?:.+) (?:= )?(#.+) :(.+)[\r\n|\r|\n]?"
			};
			std::optional<NAMES> NAMES::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t user    = 1;
				constexpr const size_t msg_id  = 2;
				constexpr const size_t channel = 3;
				constexpr const size_t list    = 4;

				using namespace std::string_literals;
				return NAMES{
					match.str(user),
					match.str(msg_id),
					match.str(channel),
					get_list_of_names(match.str(list))
				};
			}

			bool operator==(const NAMES& lhs, const NAMES& rhs) {
				return lhs.user    == rhs.user
					&& lhs.msg_id  == rhs.msg_id
					&& lhs.channel == rhs.channel
					&& lhs.names   == rhs.names;
			}

			bool operator!=(const NAMES& lhs, const NAMES& rhs) {
				return !(lhs == rhs);
			}

			const std::regex PART::regex{
				// first 3 groups are identical
				":(.+)!(?:.+)@(?:.+).tmi.twitch.tv PART (#.+)[\r\n|\r|\n]?"
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

			bool operator==(const PART& lhs, const PART& rhs) {
				return lhs.user == rhs.user
					&& lhs.channel == rhs.channel;
			}

			bool operator!=(const PART& lhs, const PART& rhs) {
				return !(lhs == rhs);
			}

		}
		namespace tags {
			const std::regex CLEARCHAT::regex{
				"@(?:ban-duration=([0-9]+);)?(?:ban-reason=([^; ]*);)?"
				"room-id=([^; ]+);(?:target-user-id=([^; ]+);)?tmi-sent-ts=([0-9]+)"
				" :tmi.twitch.tv CLEARCHAT (#[^ ]+)(?: :(.+))?[\r\n|\r|\n]?"
			};
			std::optional<CLEARCHAT> CLEARCHAT::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t duration       = 1;
				constexpr const size_t reason         = 2;
				constexpr const size_t room_id        = 3;
				constexpr const size_t target_user_id = 4;
				constexpr const size_t tmi_sent_ts    = 5;
				constexpr const size_t channel        = 6;
				constexpr const size_t user           = 7;

				return CLEARCHAT{
					commands::CLEARCHAT{
						match.str(channel), match.str(user)
					},
					get_optional<timestamp_t>(match.str(duration)),
					get_optional<std::string>(boost::replace_all_copy(match.str(reason), R"(\s)", " ")),
					match.str(room_id),
					get_optional<std::string>(match.str(target_user_id)),
					get_ts(match.str(tmi_sent_ts))
				};
			}

			CLEARCHAT::CLEARCHAT(
				commands::CLEARCHAT&&        t_plain,
				std::optional<timestamp_t>   t_duration,
				std::optional<std::string>&& t_reason,
				std::string&&                t_room_id,
				std::optional<std::string>&& t_target_user_id,
				timestamp_t                  t_tmi_sent_ts
			) :
				cap::commands::CLEARCHAT{ std::move(t_plain) },
				ban_duration(t_duration),
				ban_reason(std::move(t_reason)),
				room_id(std::move(t_room_id)),
				target_user_id(std::move(t_target_user_id)),
				tmi_sent_ts(t_tmi_sent_ts)
			{
			}

			bool operator==(const CLEARCHAT& lhs, const CLEARCHAT& rhs) {
				return lhs.ban_duration   == rhs.ban_duration
					&& lhs.ban_reason     == rhs.ban_reason
					&& lhs.room_id        == rhs.room_id
					&& lhs.target_user_id == rhs.target_user_id
					&& lhs.tmi_sent_ts    == rhs.tmi_sent_ts

					&& static_cast<commands::CLEARCHAT>(lhs)
						== static_cast<commands::CLEARCHAT>(rhs);
			}

			bool operator!=(const CLEARCHAT& lhs, const CLEARCHAT& rhs) {
				return !(lhs == rhs);
			}

			const std::regex GLOBALUSERSTATE::regex{
				"@badges=([^; ]*);color=([^; ]*);display-name=([^; ]*);"
				"emote-sets=([^; ]*);user-id=([^; ]+);user-type=([^ ]*)"
				" :tmi.twitch.tv GLOBALUSERSTATE[\r\n|\r|\n]?"
			};
			std::optional<GLOBALUSERSTATE> GLOBALUSERSTATE::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t badges       = 1;
				constexpr const size_t color        = 2;
				constexpr const size_t display_name = 3;
				constexpr const size_t emote_sets   = 4;
				constexpr const size_t user_id      = 5;
				constexpr const size_t user_type    = 6;

				return GLOBALUSERSTATE{
					get_badges(match.str(badges)),
					get_color(match.str(color)),
					match.str(display_name),
					match.str(emote_sets),
					match.str(user_id),
					UserType::from_string(match.str(user_type))
				};
			}

			bool operator==(const GLOBALUSERSTATE& lhs, const GLOBALUSERSTATE& rhs) {
				return lhs.badges       == rhs.badges
					&& lhs.color        == rhs.color
					&& lhs.display_name == rhs.display_name
					&& lhs.emote_set    == rhs.emote_set
					&& lhs.user_id      == rhs.user_id
					&& lhs.user_type    == rhs.user_type;
			}

			bool operator!=(const GLOBALUSERSTATE& lhs, const GLOBALUSERSTATE& rhs) {
				return !(lhs == rhs);
			}

			const std::regex PRIVMSG::regex{
				"@badges=([^;]*);(?:bits=([0-9]+);)?color=([^;]*);display-name=([^;]*);(?:emote-only=([01]);)?"
				"emotes=([^;]*);id=([^;]+);mod=([01]);room-id=([0-9]+);subscriber=([01]);"
				"tmi-sent-ts=([0-9]+);turbo=([01]);user-id=([0-9]+);user-type=([^ ]*)"
				" :([^!]+)!(?:[^@]+)@(?:[^.]+).([^ ]+) PRIVMSG (#.+) :(.+)[\r\n|\r|\n]?"
			};
			std::optional<PRIVMSG> PRIVMSG::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t badges       = 1;
				constexpr const size_t bits         = 2;
				constexpr const size_t color        = 3;
				constexpr const size_t display_name = 4;
				constexpr const size_t emote_only   = 5;
				constexpr const size_t emotes       = 6;
				constexpr const size_t id           = 7;
				constexpr const size_t mod          = 8;
				constexpr const size_t room_id      = 9;
				constexpr const size_t subscriber   = 10;
				constexpr const size_t tmi_sent_ts  = 11;
				constexpr const size_t turbo        = 12;
				constexpr const size_t user_id      = 13;
				constexpr const size_t user_type    = 14;
				constexpr const size_t user         = 15;
				constexpr const size_t host         = 16;
				constexpr const size_t channel      = 17;
				constexpr const size_t message      = 18;

				return PRIVMSG{
					message::PRIVMSG{
						match.str(user),
						match.str(host),
						match.str(channel),
						match.str(message)
					},
					get_badges(match.str(badges)),
					get_bits(match.str(bits)),
					get_color(match.str(color)),
					match.str(display_name),
					get_flag(match.str(emote_only)),
					match.str(emotes),
					match.str(id),
					get_flag(match.str(mod)),
					match.str(room_id),
					get_flag(match.str(subscriber)),
					get_ts(match.str(tmi_sent_ts)),
					get_flag(match.str(turbo)),
					match.str(user_id),
					UserType::from_string(match.str(user_type))
				};
			}

			PRIVMSG::PRIVMSG(
				message::PRIVMSG&&            t_plain,
				std::map<Badge, BadgeLevel>&& t_badge,
				unsigned int  t_bits,
				Color         t_color,
				std::string&& t_display_name,
				bool          t_emote_only,
				std::string&& t_emotes,
				std::string&& t_id,
				bool          t_mod,
				std::string&& t_room_id,
				bool          t_subscriber,
				timestamp_t   t_tmi_sent_ts,
				bool          t_turbo,
				std::string&& t_user_id,
				UserType      t_user_type
			) :
				message::PRIVMSG{ std::move(t_plain) },
				badges(std::move(t_badge)),
				bits(t_bits),
				color(std::move(t_color)),
				display_name(std::move(t_display_name)),
				emote_only(t_emote_only),
				emotes(std::move(t_emotes)),
				id(std::move(t_id)),
				mod(t_mod),
				room_id(std::move(t_room_id)),
				subscriber(t_subscriber),
				tmi_sent_ts(t_tmi_sent_ts),
				turbo(t_turbo),
				user_id(std::move(t_user_id)),
				user_type(t_user_type)
			{
			}

			bool operator==(const PRIVMSG& lhs, const PRIVMSG& rhs) {
				return lhs.id           == rhs.id
					&& lhs.tmi_sent_ts  == rhs.tmi_sent_ts
					&& lhs.user_id      == rhs.user_id
					&& lhs.badges       == rhs.badges
					&& lhs.bits         == rhs.bits
					&& lhs.color        == rhs.color
					&& lhs.display_name == rhs.display_name
					&& lhs.emotes       == rhs.emotes
					&& lhs.mod          == rhs.mod
					&& lhs.room_id      == rhs.room_id
					&& lhs.subscriber   == rhs.subscriber
					&& lhs.turbo        == rhs.turbo
					&& lhs.user_type    == rhs.user_type

					&& static_cast<message::PRIVMSG>(lhs)
						== static_cast<message::PRIVMSG>(rhs);
			}

			bool operator!=(const PRIVMSG& lhs, const PRIVMSG& rhs) {
				return !(lhs == rhs);
			}

			const std::regex ROOMSTATE::regex{
				"@(?:broadcaster-lang=([^; ]*);)?(?:emote-only=([01]);)?"
				"(?:followers-only=([0-9]+|-1);)?(?:r9k=([01]);)?"
				"(?:rituals=([^; ]+);)?room-id=([^; ]+);?"
				"(?:slow=([0-9]+);?)?(?:subs-only=([01]))?"
				" :tmi.twitch.tv ROOMSTATE (#.+)[\r\n|\r|\n]?"
			};
			std::optional<ROOMSTATE> ROOMSTATE::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t broadcaster_lang = 1;
				constexpr const size_t emote_only       = 2;
				constexpr const size_t followers_only   = 3;
				constexpr const size_t r9k              = 4;
				constexpr const size_t rituals          = 5;
				constexpr const size_t room_id          = 6;
				constexpr const size_t slow             = 7;
				constexpr const size_t subs_only        = 8;
				constexpr const size_t channel          = 9;

				return ROOMSTATE{
					cap::commands::ROOMSTATE{ match.str(channel) },
					get_optional<std::string>(match.str(broadcaster_lang)),
					get_optional<bool>(match.str(emote_only)),
					get_optional<int>(match.str(followers_only)),
					get_optional<bool>(match.str(r9k)),
					get_optional<std::string>(match.str(rituals)),
					match.str(room_id),
					get_optional<timestamp_t>(match.str(slow)),
					get_optional<bool>(match.str(subs_only))
				};
			}

			ROOMSTATE::ROOMSTATE(
				cap::commands::ROOMSTATE&&   t_roomstate,
				std::optional<std::string>&& t_lang,
				std::optional<bool>          t_emote_only,
				std::optional<int>           t_followers_only,
				std::optional<bool>          t_r9k,
				std::optional<std::string>&& t_rituals,
				std::string&&                t_room_id,
				std::optional<timestamp_t>   t_slow,
				std::optional<bool>          t_subs_only
			) :
				cap::commands::ROOMSTATE{ t_roomstate },
				broadcaster_lang(std::move(t_lang)),
				emote_only(t_emote_only),
				followers_only(t_followers_only),
				r9k(t_r9k),
				rituals(t_rituals),
				room_id(t_room_id),
				slow(t_slow),
				subs_only(t_subs_only)
			{
			}

			bool operator==(const ROOMSTATE& lhs, const ROOMSTATE& rhs) {
				return lhs.broadcaster_lang == rhs.broadcaster_lang
					&& lhs.emote_only == rhs.emote_only
					&& lhs.followers_only == rhs.followers_only
					&& lhs.r9k == rhs.r9k
					&& lhs.rituals == rhs.rituals
					&& lhs.room_id == rhs.room_id
					&& lhs.slow == rhs.slow
					&& lhs.subs_only == rhs.subs_only

					&& static_cast<commands::ROOMSTATE>(lhs)
						== static_cast<commands::ROOMSTATE>(rhs);
						
			}

			bool operator!=(const ROOMSTATE& lhs, const ROOMSTATE& rhs) {
				return !(lhs == rhs);
			}

			std::string USERNOTICE::ParseError::what() const noexcept {
				return err;
			}

			bool operator==(const USERNOTICE::ParseError& lhs, const USERNOTICE::ParseError& rhs) {
				return lhs.err == rhs.err;
			}

			bool operator!=(const USERNOTICE::ParseError& lhs, const USERNOTICE::ParseError& rhs) {
				return !(lhs == rhs);
			}

			const std::regex USERNOTICE::Sub::regex{
				"msg-id=(?:sub|resub);msg-param-months=([0-9]+);msg-param-sub-plan=([^;]+);msg-param-sub-plan-name=(.+)"
			};
			std::optional<USERNOTICE::Sub> USERNOTICE::Sub::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const std::size_t months        = 1;
				constexpr const std::size_t sub_plan      = 2;
				constexpr const std::size_t sub_plan_name = 3;

				return Sub{
					std::atoi(match.str(months).c_str()),
					match.str(sub_plan),
					boost::replace_all_copy(match.str(sub_plan_name), R"(\s)", " ")
				};
			}

			bool operator==(const USERNOTICE::Sub& lhs, const USERNOTICE::Sub& rhs) {
				return lhs.months == rhs.months
					&& lhs.sub_plan == rhs.sub_plan
					&& lhs.sub_plan_name == rhs.sub_plan_name;
			}

			bool operator!=(const USERNOTICE::Sub& lhs, const USERNOTICE::Sub& rhs) {
				return !(lhs == rhs);
			}

			const std::regex USERNOTICE::Subgift::regex{
				"msg-id=subgift;msg-param-months=([0-9]+);msg-param-recipient-display-name=([^;]*);"
				"msg-param-recipient-id=([^;]+);msg-param-recipient-name=([^;]+);"
				"msg-param-sub-plan-name=([^;]*);msg-param-sub-plan=(.+)"
			};
			std::optional<USERNOTICE::Subgift> USERNOTICE::Subgift::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t months                 = 1;
				constexpr const size_t recipient_display_name = 2;
				constexpr const size_t recipient_id           = 3;
				constexpr const size_t recipient_name         = 4;
				constexpr const size_t sub_plan_name          = 5;
				constexpr const size_t sub_plan               = 6;

				return Subgift{
					std::atoi(match.str(months).c_str()),
					match.str(recipient_display_name),
					match.str(recipient_id),
					match.str(recipient_name),
					boost::replace_all_copy(match.str(sub_plan_name), R"(\s)", " "),
					match.str(sub_plan),
				};
			}

			bool operator==(const USERNOTICE::Subgift& lhs, const USERNOTICE::Subgift& rhs) {
				return lhs.months                 == rhs.months
					&& lhs.recipient_display_name == rhs.recipient_display_name
					&& lhs.recipient_id           == rhs.recipient_id
					&& lhs.recipient_name         == rhs.recipient_name
					&& lhs.sub_plan_name          == rhs.sub_plan_name
					&& lhs.sub_plan               == rhs.sub_plan;
			}

			bool operator!=(const USERNOTICE::Subgift& lhs, const USERNOTICE::Subgift& rhs) {
				return !(lhs == rhs);
			}

			const std::regex USERNOTICE::Raid::regex{
				"msg-id=raid;msg-param-displayName=([^;]*);msg-param-login=([^;]+);msg-param-viewerCount=([0-9]*)"
			};
			std::optional<USERNOTICE::Raid> USERNOTICE::Raid::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t display_name = 1;
				constexpr const size_t login        = 2;
				constexpr const size_t viewer_count = 3;

				return Raid{
					match.str(display_name),
					match.str(login),
					std::atoi(match.str(viewer_count).c_str())
				};
			}

			bool operator==(const USERNOTICE::Raid& lhs, const USERNOTICE::Raid& rhs) {
				return lhs.display_name == rhs.display_name
					&& lhs.login        == rhs.login
					&& lhs.viewer_count == rhs.viewer_count;
			}

			bool operator!=(const USERNOTICE::Raid& lhs, const USERNOTICE::Raid& rhs) {
				return !(lhs == rhs);
			}

			std::optional<USERNOTICE::Ritual> USERNOTICE::Ritual::is(std::string_view raw_message) {
				using namespace std::string_view_literals;
				const auto ritual = "msg-id=ritual;msg-param-ritual-name=new_chatter"sv;
				if (raw_message != ritual) { return std::nullopt; }

				return Ritual{};
			}

			bool operator==(const USERNOTICE::Ritual& lhs, const USERNOTICE::Ritual& rhs) {
				return true;
			}

			bool operator!=(const USERNOTICE::Ritual& lhs, const USERNOTICE::Ritual& rhs) {
				return !(lhs == rhs);
			}

			const std::regex USERNOTICE::regex{
				"@badges=(.*);color=(.*);display-name=(.*);emotes=(.*);"
				"id=(.+);login=(.+);mod=([01]);(msg-id=.+);room-id=(.+);"
				"subscriber=([01]);system-msg=(.*);tmi-sent-ts=([0-9]+);turbo=([01]);"
				"user-id=(.+);user-type=(.*) :tmi.twitch.tv USERNOTICE (#[^ ]+)(?: :(.+))?[\r\n|\r|\n]?"
			};
			std::optional<USERNOTICE> USERNOTICE::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t badge        = 1;
				constexpr const size_t color        = 2;
				constexpr const size_t display_name = 3;
				constexpr const size_t emotes       = 4;
				constexpr const size_t id           = 5;
				constexpr const size_t login        = 6;
				constexpr const size_t mod          = 7;
				constexpr const size_t msg_id       = 8;
				constexpr const size_t room_id      = 9;
				constexpr const size_t subscriber   = 10;
				constexpr const size_t system_msg   = 11;
				constexpr const size_t tmi_sent_ts  = 12;
				constexpr const size_t turbo        = 13;
				constexpr const size_t user_id      = 14;
				constexpr const size_t user_type    = 15;
				constexpr const size_t channel      = 16;
				constexpr const size_t message      = 17;

				const auto get_msg_id_details =
					[&]() -> std::remove_const_t<decltype(USERNOTICE::msg_id)>
					{
						const auto raw_msg_id = match.str(msg_id);
						if (auto parsed = Sub::is(raw_msg_id))     { return std::move(*parsed); }
						if (auto parsed = Subgift::is(raw_msg_id)) { return std::move(*parsed); }
						if (auto parsed = Raid::is(raw_msg_id))    { return std::move(*parsed); }
						if (auto parsed = Ritual::is(raw_msg_id))  { return *parsed; }
					
						return ParseError{};
					};
				
				return USERNOTICE{
					cap::commands::USERNOTICE{ match.str(channel), match.str(message) },
					get_badges(match.str(badge)),
					get_color(match.str(color)),
					match.str(display_name),
					match.str(emotes),
					match.str(id),
					match.str(login),
					get_flag(match.str(mod)),
					get_msg_id_details(),
					match.str(room_id),
					get_flag(match.str(subscriber)),
					boost::replace_all_copy(match.str(system_msg), R"(\s)", " "),
					get_ts(match.str(tmi_sent_ts)),
					get_flag(match.str(turbo)),
					match.str(user_id),
					UserType::from_string(match.str(user_type))
				};
			}

			USERNOTICE::USERNOTICE(
				cap::commands::USERNOTICE&&   t_usernotice,
				std::map<Badge, BadgeLevel>&& t_badges,
				Color         t_color,
				std::string&& t_display_name,
				std::string&& t_emotes,
				std::string&& t_id,
				std::string&& t_login,
				bool          t_mod,
				boost::variant<ParseError, Sub, Subgift, Raid, Ritual>&& t_msg_id,
				std::string&& t_room_id,
				bool          t_subscriber,
				std::string&& t_system_msg,
				timestamp_t   t_tmi_sent_ts,
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
				tmi_sent_ts(t_tmi_sent_ts),
				turbo(t_turbo),
				user_id(std::move(t_user_id)),
				user_type(t_user_type)
			{
			}

			bool operator==(const USERNOTICE& lhs, const USERNOTICE& rhs) {
				return lhs.id           == rhs.id
					&& lhs.tmi_sent_ts  == rhs.tmi_sent_ts
					&& lhs.user_id      == rhs.user_id
					&& lhs.login        == rhs.login
					&& lhs.badges       == rhs.badges
					&& lhs.color        == rhs.color
					&& lhs.display_name == rhs.display_name
					&& lhs.emotes       == rhs.emotes
					&& lhs.mod          == rhs.mod
					&& lhs.msg_id       == rhs.msg_id
					&& lhs.room_id      == rhs.room_id
					&& lhs.subscriber   == rhs.subscriber
					&& lhs.system_msg   == rhs.system_msg
					&& lhs.turbo        == rhs.turbo
					&& lhs.user_type    == rhs.user_type

					&& static_cast<commands::USERNOTICE>(lhs)
						== static_cast<commands::USERNOTICE>(rhs);
			}

			bool operator!=(const USERNOTICE& lhs, const USERNOTICE& rhs) {
				return !(lhs == rhs);
			}

			const std::regex USERSTATE::regex{
				"@badges=([^; ]*);color=([^; ]*);display-name=([^; ]*);"
				"emote-sets=([^; ]*);mod=([01]);subscriber=([01]);"
				"user-type=([^; ]*)"
				" :tmi.twitch.tv USERSTATE (#.+)[\r\n|\r|\n]?"
			};
			std::optional<USERSTATE> USERSTATE::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const size_t badges       = 1;
				constexpr const size_t color        = 2;
				constexpr const size_t display_name = 3;
				constexpr const size_t emote_sets   = 4;
				constexpr const size_t mod          = 5;
				constexpr const size_t subscriber   = 6;
				constexpr const size_t user_type    = 7;
				constexpr const size_t channel      = 8;

				using namespace std::string_literals;
				return USERSTATE{
					cap::commands::USERSTATE{ match.str(channel) },
					get_badges(match.str(badges)),
					get_color(match.str(color)),
					match.str(display_name),
					match.str(emote_sets),
					get_flag(match.str(mod)),
					get_flag(match.str(subscriber)),
					UserType::from_string(match.str(user_type))
				};
			}

			USERSTATE::USERSTATE(
				cap::commands::USERSTATE&&    t_userstate,
				std::map<Badge, BadgeLevel>&& t_badges,
				Color         t_color,
				std::string&& t_display_name,
				std::string&& t_emote_sets,
				bool          t_mod,
				bool          t_subscriber,
				UserType      t_user_type
			) :
				cap::commands::USERSTATE{ std::move(t_userstate) },
				badges(std::move(t_badges)),
				color(std::move(t_color)),
				display_name(std::move(t_display_name)),
				emote_sets(std::move(t_emote_sets)),
				mod(t_mod),
				subscriber(t_subscriber),
				user_type(t_user_type)
			{
			}

			bool operator==(const USERSTATE& lhs, const USERSTATE& rhs) {
				return lhs.badges == rhs.badges
					&& lhs.color == rhs.color
					&& lhs.display_name == rhs.display_name
					&& lhs.emote_sets == rhs.emote_sets
					&& lhs.mod == rhs.mod
					&& lhs.subscriber == rhs.subscriber
					&& lhs.user_type == rhs.user_type

					&& static_cast<commands::USERSTATE>(lhs)
						== static_cast<commands::USERSTATE>(rhs);
			}

			bool operator!=(const USERSTATE& lhs, const USERSTATE& rhs) {
				return !(lhs == rhs);
			}

		}
		namespace commands {
			const std::regex CLEARCHAT::regex{
				":tmi.twitch.tv CLEARCHAT (#[^ ]+)(?: :(.+))?[\r\n|\r|\n]?"
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

			bool operator==(const CLEARCHAT& lhs, const CLEARCHAT& rhs) {
				return lhs.channel == rhs.channel
					&& lhs.user    == rhs.user;
			}

			bool operator!=(const CLEARCHAT& lhs, const CLEARCHAT& rhs) {
				return !(lhs == rhs);
			}

			const std::regex HOSTTARGET::regex{
				R"(:tmi.twitch.tv HOSTTARGET (#[^ ]+) (?:([^ :]+)|:-) (?:\[([0-9]*)\])?[\r\n|\r|\n]?)"
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
					get_optional<int>(match.str(viewers_count))
				};
			}

			bool operator==(const HOSTTARGET& lhs, const HOSTTARGET& rhs) {
				return lhs.hosting_channel == rhs.hosting_channel
					&& lhs.target_channel  == rhs.target_channel
					&& lhs.viewers_count   == rhs.viewers_count;
			}

			bool operator!=(const HOSTTARGET& lhs, const HOSTTARGET& rhs) {
				return !(lhs == rhs);
			}

			const std::regex NOTICE::regex{
				"@msg-id=(.+) :tmi.twitch.tv NOTICE (#.+) :(.+)[\r\n|\r|\n]?"
			};
			std::optional<NOTICE> NOTICE::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const std::size_t msg_id  = 1;
				constexpr const std::size_t channel = 2;
				constexpr const std::size_t message = 3;

				return NOTICE{
					match.str(msg_id),
					match.str(channel),
					match.str(message)
				};
			}

			bool operator==(const NOTICE& lhs, const NOTICE& rhs) {
				return lhs.msg_id  == rhs.msg_id
					&& lhs.channel == rhs.channel
					&& lhs.message == rhs.message;
			}

			bool operator!=(const NOTICE& lhs, const NOTICE& rhs) {
				return !(lhs == rhs);
			}

			const std::regex RECONNECT::regex{
				"RECONNECT[\r\n|\r|\n]?"
			};
			std::optional<RECONNECT> RECONNECT::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				return RECONNECT{};
			}

			bool operator==(const RECONNECT& lhs, const RECONNECT& rhs) {
				return true;
			}

			bool operator!=(const RECONNECT& lhs, const RECONNECT& rhs) {
				return !(lhs == rhs);
			}

			const std::regex ROOMSTATE::regex{
				":tmi.twitch.tv ROOMSTATE (#.+)[\r\n|\r|\n]?"
			};
			std::optional<ROOMSTATE> ROOMSTATE::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const std::size_t channel = 1;

				return ROOMSTATE{
					match.str(channel)
				};
			}
			
			bool operator==(const ROOMSTATE& lhs, const ROOMSTATE& rhs) {
				return lhs.channel == rhs.channel;
			}

			bool operator!=(const ROOMSTATE& lhs, const ROOMSTATE& rhs) {
				return !(lhs == rhs);
			}

			const std::regex USERNOTICE::regex{
				":tmi.twitch.tv USERNOTICE (#.+) :(.+)[\r\n|\r|\n]?"
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

			bool operator==(const USERNOTICE& lhs, const USERNOTICE& rhs) {
				return lhs.channel == rhs.channel
					&& lhs.message == rhs.message;
			}

			bool operator!=(const USERNOTICE& lhs, const USERNOTICE& rhs) {
				return !(lhs == rhs);
			}

			const std::regex USERSTATE::regex{
				":tmi.twitch.tv USERSTATE (#.+)[\r\n|\r|\n]?"
			};
			std::optional<USERSTATE> USERSTATE::is(std::string_view raw_message) {
				std::cmatch match;
				if (!std::regex_match(raw_message.data(), match, regex)) { return std::nullopt; }

				constexpr const std::size_t channel = 1;

				return USERSTATE{
					match.str(channel)
				};
			}

			bool operator==(const USERSTATE& lhs, const USERSTATE& rhs) {
				return lhs.channel == rhs.channel;
			}

			bool operator!=(const USERSTATE& lhs, const USERSTATE& rhs) {
				return !(lhs == rhs);
			}

		} // namespace commands
	} // namespace cap

	MessageParser::result_t MessageParser::process(std::string_view recived_message) {
		try
		{
			namespace commands   = Twitch::irc::message::cap::commands;
			namespace membership = Twitch::irc::message::cap::membership;
			namespace tags       = Twitch::irc::message::cap::tags;
			// TODO: simplify, iterate through types
			// sorted by priority
			if (auto parsed = PING::is(recived_message); parsed) {
				return *parsed;
			}
			if (auto parsed = tags::PRIVMSG::is(recived_message); parsed) {
				return *parsed;
			}
			if (auto parsed = membership::JOIN::is(recived_message); parsed) {
				return *parsed;
			}
			if (auto parsed = membership::PART::is(recived_message); parsed) {
				return *parsed;
			}
			if (auto parsed = tags::CLEARCHAT::is(recived_message); parsed) {
				return *parsed;
			}
			if (auto parsed = tags::USERNOTICE::is(recived_message); parsed) {
				return *parsed;
			}
			if (auto parsed = commands::NOTICE::is(recived_message); parsed) {
				return *parsed;
			}
			if (auto parsed = tags::USERSTATE::is(recived_message); parsed) {
				return *parsed;
			}

			// only once or very rarely
			if (auto parsed = membership::MODE::is(recived_message); parsed) {
				return *parsed;
			}
			if (auto parsed = commands::HOSTTARGET::is(recived_message); parsed) {
				return *parsed;
			}
			if (auto parsed = tags::GLOBALUSERSTATE::is(recived_message); parsed) {
				return *parsed;
			}
			if (auto parsed = tags::ROOMSTATE::is(recived_message); parsed) {
				return *parsed;
			}
			if (auto parsed = membership::NAMES::is(recived_message); parsed) {
				return *parsed;
			}
			if (auto parsed = commands::RECONNECT::is(recived_message); parsed) {
				return *parsed;
			}

			using namespace std::string_literals;
			return ParseError{ "Message type not handled: "s + recived_message.data() };
		}
		catch (const std::regex_error& e) {
			return ParseError{ e.what() };
		}
		catch (const std::exception& e) {
			return ParseError{ e.what() };
		}
		catch (...) {
			using namespace std::string_literals;
			return ParseError{ "Unknown exception"s };
		}
	}

	MessageParser::MessageParser(std::shared_ptr<IController> irc_controller)
		: m_controller(std::move(irc_controller))
	{
	}
}