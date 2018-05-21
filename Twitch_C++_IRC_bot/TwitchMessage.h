#ifndef TWITCHMESSAGE_H
#define TWITCHMESSAGE_H
#include "Logger.h"
#include "IRC_Bot.h"
#include <boost\log\trivial.hpp>
#include <boost\variant.hpp>
#include <boost\algorithm\string\predicate.hpp>
#include <ctime>
#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

class ThreadSafeLogger;

namespace Twitch {
	namespace irc {
		struct Commands;
		struct IRCWriter;
		struct IController;
		class Controller;
	} // namespace irc
} // namespace Twitch

namespace Twitch::irc::message {
	struct PING
	{
		static const std::regex regex;
		static std::optional<PING> is(std::string_view raw_message);

		std::string host;

		friend bool operator==(const PING& lhs, const PING& rhs);
		friend bool operator!=(const PING& lhs, const PING& rhs);

		template<class Logger>
		friend Logger& operator<<(Logger& logger, const PING& msg);
	};
	struct PRIVMSG
	{
		static const std::regex regex;
		static std::optional<PRIVMSG> is(std::string_view raw_message);

		const std::string user;
		const std::string host;
		const std::string channel;
		const std::string message;

		friend bool operator==(const PRIVMSG& lhs, const PRIVMSG& rhs);
		friend bool operator!=(const PRIVMSG& lhs, const PRIVMSG& rhs);

		template<class Logger>
		friend Logger& operator<<(Logger& logger, const PRIVMSG& msg);
	};

	template<class Logger>
	Logger& operator<<(Logger& logger, const PING& msg) {
		return logger << "PING :" << msg.host;
	}
	template<class Logger>
	Logger& operator<<(Logger& logger, const PRIVMSG& msg) {
		return logger << ':'
			<< msg.user << '!'
			<< msg.user << '@'
			<< msg.host
			<< msg.channel << " :"
			<< msg.message;
	}
	
	/// https://dev.twitch.tv/docs/irc#twitch-specific-irc-capabilities
	namespace cap {
		/// https://dev.twitch.tv/docs/irc#twitch-irc-capability-commands
		namespace commands {
			/// raw, extended by cap tags
			struct CLEARCHAT
			{
				static const std::regex regex;
				static std::optional<CLEARCHAT> is(std::string_view raw_message);
				
				const std::string channel;
				const std::string user;

				friend bool operator==(const CLEARCHAT& lhs, const CLEARCHAT& rhs);
				friend bool operator!=(const CLEARCHAT& lhs, const CLEARCHAT& rhs);

				template<class Logger>
				friend Logger& operator<<(Logger& logger, const CLEARCHAT& msg);
			};
			struct HOSTTARGET
			{
				static const std::regex regex;
				static std::optional<HOSTTARGET> is(std::string_view raw_message);

				inline bool starts() const noexcept {
					return !target_channel.empty();
				}

				const std::string hosting_channel;
				const std::string target_channel;
				const std::optional<int> viewers_count;

				friend bool operator==(const HOSTTARGET& lhs, const HOSTTARGET& rhs);
				friend bool operator!=(const HOSTTARGET& lhs, const HOSTTARGET& rhs);

				template<class Logger>
				friend Logger& operator<<(Logger& logger, const HOSTTARGET& msg);
			};
			struct NOTICE
			{
				static const std::regex regex;
				static std::optional<NOTICE> is(std::string_view raw_message);

				const std::string msg_id;
				const std::string channel;
				const std::string message;

				friend bool operator==(const NOTICE& lhs, const NOTICE& rhs);
				friend bool operator!=(const NOTICE& lhs, const NOTICE& rhs);

				template<class Logger>
				friend Logger& operator<<(Logger& logger, const NOTICE& msg);
			};
			struct RECONNECT
			{
				static const std::regex regex;
				static std::optional<RECONNECT> is(std::string_view raw_message);

				friend bool operator==(const RECONNECT& lhs, const RECONNECT& rhs);
				friend bool operator!=(const RECONNECT& lhs, const RECONNECT& rhs);

				template<class Logger>
				friend Logger& operator<<(Logger& logger, const RECONNECT& msg);
			};
			/// raw, extended by cap tags
			struct ROOMSTATE
			{
				static const std::regex regex;
				static std::optional<ROOMSTATE> is(std::string_view raw_message);
				
				const std::string channel;

				friend bool operator==(const ROOMSTATE& lhs, const ROOMSTATE& rhs);
				friend bool operator!=(const ROOMSTATE& lhs, const ROOMSTATE& rhs);

				template<class Logger>
				friend Logger& operator<<(Logger& logger, const ROOMSTATE& msg);
			};
			/// raw, extended by cap tags
			struct USERNOTICE
			{
				static const std::regex regex;
				static std::optional<USERNOTICE> is(std::string_view raw_message);

				const std::string channel;
				const std::string message;

				friend bool operator==(const USERNOTICE& lhs, const USERNOTICE& rhs);
				friend bool operator!=(const USERNOTICE& lhs, const USERNOTICE& rhs);

				template<class Logger>
				friend Logger& operator<<(Logger& logger, const USERNOTICE& msg);
			};
			/// raw, extended by cap tags
			struct USERSTATE
			{
				static const std::regex regex;
				static std::optional<USERSTATE> is(std::string_view raw_message);

				const std::string channel;

				friend bool operator==(const USERSTATE& lhs, const USERSTATE& rhs);
				friend bool operator!=(const USERSTATE& lhs, const USERSTATE& rhs);

				template<class Logger>
				friend Logger& operator<<(Logger& logger, const USERSTATE& msg);
			};

			template<class Logger>
			Logger& operator<<(Logger& logger, const CLEARCHAT& msg) {
				logger << ":tmi.twitch.tv CLEARCHAT " << msg.channel;
				if (!msg.user.empty()) {
					logger << " :" << msg.user;
				}
				return logger;
			}
			template<class Logger>
			Logger& operator<<(Logger& logger, const HOSTTARGET& msg) {
				logger << ":tmi.twitch.tv HOSTTARGET "
					<< msg.hosting_channel << ' '
					<< (msg.starts() ? msg.target_channel : ":-");

				if (msg.viewers_count) {
					logger << " [" << msg.viewers_count.value() << ']';
				}
				return logger;
			}
			template<class Logger>
			Logger& operator<<(Logger& logger, const NOTICE& msg) {
				return logger << "@msg-id=" << msg.msg_id
					<< ":tmi.twitch.tv NOTICE " << msg.channel
					<< " :" << msg.message;
			}
			template<class Logger>
			Logger& operator<<(Logger& logger, const RECONNECT& msg) {
				return logger << "RECONNECT";
			}
			template<class Logger>
			Logger& operator<<(Logger& logger, const ROOMSTATE& msg) {
				return logger << ":tmi.twitch.tv ROOMSTATE " << msg.channel;
			}
			template<class Logger>
			Logger& operator<<(Logger& logger, const USERNOTICE& msg) {
				return logger << ":tmi.twitch.tv USERNOTICE " << msg.channel
					<< " :" << msg.message;
			}
			template<class Logger>
			Logger& operator<<(Logger& logger, const USERSTATE& msg) {
				return logger << ":tmi.twitch.tv USERSTATE " << msg.channel;
			}
		} // namespace commands

		/// https://dev.twitch.tv/docs/irc#twitch-irc-capability-membership
		namespace membership {
			struct JOIN
			{
				static const std::regex regex;
				static std::optional<JOIN> is(std::string_view raw_message);

				const std::string user;
				const std::string channel;

				friend bool operator==(const JOIN& lhs, const JOIN& rhs);
				friend bool operator!=(const JOIN& lhs, const JOIN& rhs);

				template<class Logger>
				friend Logger& operator<<(Logger& logger, const JOIN& msg);
			};
			struct MODE
			{
				static const std::regex regex;
				static std::optional<MODE> is(std::string_view raw_message);

				const std::string channel;
				const bool gained; // true == +o; false == -o
				const std::string user;

				friend bool operator==(const MODE& lhs, const MODE& rhs);
				friend bool operator!=(const MODE& lhs, const MODE& rhs);

				template<class Logger>
				friend Logger& operator<<(Logger& logger, const MODE& msg);
			};
			struct NAMES
			{
				static const std::regex regex;
				static std::optional<NAMES> is(std::string_view raw_message);

				inline bool is_end_of_list() const noexcept {
					using namespace std::string_literals;
					return msg_id == "366"s;
				}

				const std::string user;
				const std::string msg_id;
				const std::string channel;
				const std::vector<std::string> names;

				friend bool operator==(const NAMES& lhs, const NAMES& rhs);
				friend bool operator!=(const NAMES& lhs, const NAMES& rhs);

				template<class Logger>
				friend Logger& operator<<(Logger& logger, const NAMES& msg);
			};
			struct PART
			{
				static const std::regex regex;
				static std::optional<PART> is(std::string_view raw_message);

				const std::string user;
				const std::string channel;

				friend bool operator==(const PART& lhs, const PART& rhs);
				friend bool operator!=(const PART& lhs, const PART& rhs);

				template<class Logger>
				friend Logger& operator<<(Logger& logger, const PART& msg);
			};

			template<class Logger>
			Logger& operator<<(Logger& logger, const JOIN& msg) {
				return logger << ':' << msg.user << '!' << msg.user << '@'
					<< msg.user << ".tmi.twitch.tv JOIN " << msg.channel;
			}
			template<class Logger>
			Logger& operator<<(Logger& logger, const MODE& msg) {
				return logger << ":jtv MODE " << msg.channel
					<< (msg.gained ? '+' : '-') << "o " << msg.user;
			}
			template<class Logger>
			Logger& operator<<(Logger& logger, const NAMES& msg) {
				if (msg.is_end_of_list()) {
					return logger
						<< ':' << msg.user << ".tmi.twitch.tv " << msg.msg_id << ' '
						<< msg.channel << " :End of /NAMES list";
				}

				logger << ':' << msg.user << ".tmi.twitch.tv " << msg.msg_id << ' '
					<< msg.user << " = " << msg.channel << " :";

				const auto first = std::begin(msg.names);
				logger << *first;
				for (auto it = std::next(first); it != std::end(msg.names); ++it) {
					logger << ' ' << *it;
				}
				return logger;
			}
			template<class Logger>
			Logger& operator<<(Logger& logger, const PART& msg) {
				return logger << ':' << msg.user << '!' << msg.user << '@'
					<< msg.user << ".tmi.twitch.tv PART " << msg.channel;
			}
		} // namespace membership

		/// https://dev.twitch.tv/docs/irc#twitch-irc-capability-tags
		namespace tags {
			struct Color {
				bool initialized{ false };
				const int r{ 0 }, g{ 0 }, b{ 0 };

				static inline auto from_string(
					const std::string& r,
					const std::string& g,
					const std::string& b
				);

				Color() = default;
				constexpr Color(int t_r, int t_g, int t_b) noexcept;

				friend bool operator==(Color lhs, Color rhs) {
					if (!(lhs.initialized && rhs.initialized)) { return false; }

					return lhs.r == rhs.r
						&& lhs.g == rhs.g
						&& lhs.b == rhs.b;
				}
				friend bool operator!=(Color lhs, Color rhs) {
					return !(lhs == rhs);
				}
			};

			auto Color::from_string(
				const std::string& r,
				const std::string& g,
				const std::string& b
			) {
				auto hex_to_int = [](const std::string& raw_hex) {
					return std::stoi(raw_hex, 0, 16);
				};

				return Color{ hex_to_int(r), hex_to_int(g), hex_to_int(b) };
			}

			constexpr Color::Color(int t_r, int t_g, int t_b) noexcept
				: initialized(true), r(t_r), g(t_g), b(t_b)
			{
			}

			// TODO: find missig badges and add
			using BadgeLevel = int;
			struct Badge{
				enum Type {
					unhandled_badge = -1,
					bits, turbo, subscriber, moderator, broadcaster, global_mod, admin, staff
				};

				const Type type;

				constexpr Badge(Type t) : type(t) {}

				friend constexpr bool operator==(Badge lhs, Badge rhs) noexcept {
					return lhs.type == rhs.type;
				}
				friend constexpr bool operator!=(Badge lhs, Badge rhs) noexcept {
					return !(lhs.type == rhs.type);
				}
				friend constexpr bool operator>(Badge lhs, Badge rhs) noexcept {
					return lhs.type > rhs.type;
				}
				friend constexpr bool operator<(Badge lhs, Badge rhs) noexcept {
					return lhs.type < rhs.type;
				}

				static constexpr auto from_string(std::string_view raw) noexcept;
				static constexpr auto to_string(Badge badge) noexcept;
			};

			constexpr auto Badge::from_string(std::string_view raw) noexcept {
				using namespace std::string_view_literals;
				if (raw == "subscriber"sv)  { return Badge::subscriber ; }
				if (raw == "bits"sv)        { return Badge::bits       ; }
				if (raw == "turbo"sv)       { return Badge::turbo      ; }
				if (raw == "moderator"sv)   { return Badge::moderator  ; }
				if (raw == "broadcaster"sv) { return Badge::broadcaster; }
				if (raw == "global_mod"sv)  { return Badge::global_mod ; }
				if (raw == "admin"sv)       { return Badge::admin      ; }
				if (raw == "staff"sv)       { return Badge::staff      ; }
					
				return Badge::unhandled_badge;
			}
			constexpr auto Badge::to_string(Badge badge) noexcept {
				if (badge == Badge::subscriber)  { return "subscriber" ; }
				if (badge == Badge::bits)        { return "bits"       ; }
				if (badge == Badge::turbo)       { return "turbo"      ; }
				if (badge == Badge::moderator)   { return "moderator"  ; }
				if (badge == Badge::broadcaster) { return "broadcaster"; }
				if (badge == Badge::global_mod)  { return "global_mod" ; }
				if (badge == Badge::admin)       { return "admin"      ; }
				if (badge == Badge::staff)       { return "staff"      ; }

				return "unhandled_badge";
			}

			struct UserType {
				enum Type {
					unhandled_type = -1,
					empty, // default
					mod, global_mod, admin, staff
				};

				const Type type{ empty };

				constexpr UserType() = default;
				constexpr UserType(Type t) : type(t) {}

				friend constexpr bool operator==(UserType lhs, UserType rhs) noexcept {
					return lhs.type == rhs.type;
				}
				friend constexpr bool operator!=(UserType lhs, UserType rhs) noexcept {
					return !(lhs.type == rhs.type);
				}

				static constexpr auto from_string(std::string_view raw) noexcept;
				static constexpr auto to_string(UserType utype) noexcept;
			};

			constexpr auto UserType::from_string(std::string_view raw) noexcept {
				using namespace std::string_view_literals;
				// most common case
				if (raw.empty())           { return UserType::empty     ; }
				if (raw == "mod"sv)        { return UserType::mod       ; }
				if (raw == "global_mod"sv) { return UserType::global_mod; }
				if (raw == "admin"sv)      { return UserType::admin     ; }
				if (raw == "staff"sv)      { return UserType::staff     ; }

				return UserType::unhandled_type;
			}
			constexpr auto UserType::to_string(UserType utype) noexcept {
				if (utype == UserType::empty)      { return ""           ; }
				if (utype == UserType::mod)        { return "mod"        ; }
				if (utype == UserType::global_mod) { return "global_mod" ; }
				if (utype == UserType::admin)      { return "admin"      ; }
				if (utype == UserType::staff)      { return "staff"      ; }
		
				return "unhandled_type";
			}

			struct CLEARCHAT : public cap::commands::CLEARCHAT
			{
				static const std::regex regex;
				static std::optional<CLEARCHAT> is(std::string_view raw_message);

				inline bool is_perm() const noexcept {
					return ban_duration == std::chrono::seconds{ 0 }
						&& !target_user_id.has_value()
						&& !user.empty();
				}
				
				inline bool is_timeout() const noexcept {
					return ban_duration > std::chrono::seconds{ 0 }
						&& !target_user_id.has_value()
						&& !user.empty();
				}

				inline bool is_clear() const noexcept {
					return user.empty();
				}

				const std::optional<std::chrono::seconds> ban_duration{ 0 }; // default == permanent
				const std::optional<std::string> ban_reason;
				const std::string                room_id;
				const std::optional<std::string> target_user_id;
				const std::chrono::seconds       tmi_sent_ts;
				
				CLEARCHAT(
					commands::CLEARCHAT&&        t_plain,
					std::optional<std::chrono::seconds> t_duration,
					std::optional<std::string>&& t_reason,
					std::string&&                t_room_id,
					std::optional<std::string>&& t_target_user_id,
					std::chrono::seconds         t_tmi_sent_ts
				);

				friend bool operator==(const CLEARCHAT& lhs, const CLEARCHAT& rhs);
				friend bool operator!=(const CLEARCHAT& lhs, const CLEARCHAT& rhs);

				template<class Logger>
				friend Logger& operator<<(Logger& logger, const CLEARCHAT& msg);
			};
			struct GLOBALUSERSTATE
			{
				static const std::regex regex;
				static std::optional<GLOBALUSERSTATE> is(std::string_view raw_message);

				const std::map<Badge, BadgeLevel> badges;
				const Color color;
				const std::string display_name;
				const std::string emote_set;
				const std::string user_id;
				const UserType user_type;

				friend bool operator==(const GLOBALUSERSTATE& lhs, const GLOBALUSERSTATE& rhs);
				friend bool operator!=(const GLOBALUSERSTATE& lhs, const GLOBALUSERSTATE& rhs);

				template<class Logger>
				friend Logger& operator<<(Logger& logger, const GLOBALUSERSTATE& msg);
			};
			struct PRIVMSG : public message::PRIVMSG
			{ // TODO: add a way to check user privileges
				static const std::regex regex;
				static std::optional<PRIVMSG> is(std::string_view raw_message);

				inline bool is_bitsmsg() const noexcept {
					return bits != 0;
				}

				const std::map<Badge, BadgeLevel> badges;
				const unsigned int bits{ 0 }; // default == not bits msg
				const Color color;
				const std::string display_name;
				const bool emote_only{ false };
				const std::string emotes; // list of emotes and their pos in message, left unprocessed
				const std::string id;
				const bool        mod;
				const std::string room_id;
				const bool        subscriber;
				const std::chrono::seconds tmi_sent_ts;
				const bool        turbo;
				const std::string user_id;
				const UserType user_type;

				PRIVMSG(
					message::PRIVMSG&& t_plain,
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
					std::chrono::seconds t_tmi_sent_ts,
					bool          t_turbo,
					std::string&& t_user_id,
					UserType      t_user_type
				);

				friend bool operator==(const PRIVMSG& lhs, const PRIVMSG& rhs);
				friend bool operator!=(const PRIVMSG& lhs, const PRIVMSG& rhs);

				template<class Logger>
				friend Logger& operator<<(Logger& logger, const PRIVMSG& msg);
			};
			struct ROOMSTATE : cap::commands::ROOMSTATE
			{
				static const std::regex regex;
				static std::optional<ROOMSTATE> is(std::string_view raw_message);

				inline bool is_update() const noexcept {
					return 1 == static_cast<int>(broadcaster_lang.has_value())
					            + static_cast<int>(emote_only.has_value())
					            + static_cast<int>(followers_only.has_value())
					            + static_cast<int>(r9k.has_value())
					            + static_cast<int>(rituals.has_value())
					            + static_cast<int>(slow.has_value())
					            + static_cast<int>(subs_only.has_value());
				}

				// empty if not enabled
				const std::optional<std::string> broadcaster_lang{ std::nullopt };
				const std::optional<bool>        emote_only;
				const std::optional<int>         followers_only; // -1 == disabled
				const std::optional<bool>        r9k;
				const std::optional<std::string> rituals; // doc doesn't say a word... std::string for safety
				const std::string                room_id;
				const std::optional<std::chrono::seconds> slow;
				const std::optional<bool>        subs_only;
				
				ROOMSTATE(
					cap::commands::ROOMSTATE&&   t_roomstate,
					std::optional<std::string>&& t_lang,
					std::optional<bool>          t_emote_only,
					std::optional<int>           t_followers_only,
					std::optional<bool>          t_r9k,
					std::optional<std::string>&& t_rituals,
					std::string&&                t_room_id,
					std::optional<std::chrono::seconds> t_slow,
					std::optional<bool>          t_subs_only
				);

				friend bool operator==(const ROOMSTATE& lhs, const ROOMSTATE& rhs);
				friend bool operator!=(const ROOMSTATE& lhs, const ROOMSTATE& rhs);

				template<class Logger>
				friend Logger& operator<<(Logger& logger, const ROOMSTATE& msg);
			};
			struct USERNOTICE : public cap::commands::USERNOTICE
			{
				struct ParseError {
					const std::string err;

					std::string what() const noexcept;

					friend bool operator==(const ParseError& lhs, const ParseError& rhs);
					friend bool operator!=(const ParseError& lhs, const ParseError& rhs);

				};
				struct Sub
				{
					static const std::regex regex;
					static std::optional<Sub> is(std::string_view raw_message);

					const int months;
					const std::string sub_plan;
					const std::string sub_plan_name;

					friend bool operator==(const Sub& lhs, const Sub& rhs);
					friend bool operator!=(const Sub& lhs, const Sub& rhs);

				};
				struct Subgift
				{
					static const std::regex regex;
					static std::optional<Subgift> is(std::string_view raw_message);

					const int months;
					const std::string recipient_display_name;
					const std::string recipient_id;
					const std::string recipient_name;
					const std::string sub_plan_name;
					const std::string sub_plan;

					friend bool operator==(const Subgift& lhs, const Subgift& rhs);
					friend bool operator!=(const Subgift& lhs, const Subgift& rhs);

				};
				struct Raid
				{
					static const std::regex regex;
					static std::optional<Raid> is(std::string_view raw_message);

					const std::string display_name;
					const std::string login;
					const int viewer_count;

					friend bool operator==(const Raid& lhs, const Raid& rhs);
					friend bool operator!=(const Raid& lhs, const Raid& rhs);

				};
				struct Ritual
				{
					static std::optional<Ritual> is(std::string_view raw_message);

					friend bool operator==(const Ritual& lhs, const Ritual& rhs);
					friend bool operator!=(const Ritual& lhs, const Ritual& rhs);

				};

				static const std::regex regex;
				static std::optional<USERNOTICE> is(std::string_view raw_message);

				const std::map<Badge, BadgeLevel> badges;
				const Color       color;
				const std::string display_name;
				const std::string emotes;
				const std::string id;
				const std::string login;
				const bool        mod;
				const boost::variant<ParseError, Sub, Subgift, Raid, Ritual> msg_id;
				const std::string room_id;
				const bool        subscriber;
				const std::string system_msg;
				const std::chrono::seconds tmi_sent_ts;
				const bool        turbo;
				const std::string user_id;
				const UserType    user_type;

				USERNOTICE(
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
					std::chrono::seconds t_tmi_sent_ts,
					bool          t_turbo,
					std::string&& t_user_id,
					UserType      t_user_type
				);

				friend bool operator==(const USERNOTICE& lhs, const USERNOTICE& rhs);
				friend bool operator!=(const USERNOTICE& lhs, const USERNOTICE& rhs);

				template<class Logger>
				friend Logger& operator<<(Logger& logger, const USERNOTICE& msg);
			};
			struct USERSTATE : public cap::commands::USERSTATE
			{
				static const std::regex regex;
				static std::optional<USERSTATE> is(std::string_view raw_message);

				const std::map<Badge, BadgeLevel> badges;
				const Color       color;
				const std::string display_name;
				const std::string emote_sets;
				const bool        mod;
				const bool        subscriber;
				const UserType    user_type;

				USERSTATE(
					cap::commands::USERSTATE&&    t_userstate,
					std::map<Badge, BadgeLevel>&& t_badges,
					Color         t_color,
					std::string&& t_display_name,
					std::string&& t_emote_sets,
					bool          t_mod,
					bool          t_subscriber,
					UserType      t_user_type
				);

				friend bool operator==(const USERSTATE& lhs, const USERSTATE& rhs);
				friend bool operator!=(const USERSTATE& lhs, const USERSTATE& rhs);

				template<class Logger>
				friend Logger& operator<<(Logger& logger, const USERSTATE& msg);
			};

			// Badges & UserType::Type
			template<class Logger>
			Logger& operator<<(Logger& logger, Color color) {
				if (!color.initialized) { return logger; }
				return logger << '#'
					<< std::hex << std::uppercase
						<< (color.r <= 0xF ? "0" : "") << color.r
						<< (color.g <= 0xF ? "0" : "") << color.g
						<< (color.b <= 0xF ? "0" : "") << color.b
					<< std::dec << std::nouppercase;
			}
			template<class Logger>
			Logger& operator<<(Logger& logger, Badge badge) {
				return logger << Badge::to_string(badge);
			}
			template<class Logger>
			Logger& operator<<(Logger& logger, const std::map<Badge, BadgeLevel>& badges) {
				const auto first = std::begin(badges);
				const auto end = std::end(badges);

				if (first == end) { return logger; }

				logger << first->first << '/' << first->second;
				for (auto it = std::next(first); it != end; ++it) {
					logger << ' ' << it->first << '/' << it->second;
				}

				return logger;
			}
			template<class Logger>
			Logger& operator<<(Logger& logger, UserType utype) {
				return logger << UserType::to_string(utype);
			}
			//

			template<class Logger>
			Logger& operator<<(Logger& logger, const CLEARCHAT& msg) {
				logger << '@';
				if (msg.ban_duration) {
					logger << "ban-duration=" << msg.ban_duration.value() << ';';
				}
				if (msg.ban_reason) {
					logger << "ban-reason="
						<< boost::replace_all_copy(msg.ban_reason.value(), " ", R"(\s)") << ';';
				}

				logger << "room-id=" << msg.room_id << ';';
				if (msg.target_user_id) {
					logger << "target-user-id=" << msg.target_user_id.value() << ';';
				}

				return logger << "tmi-sent-ts=" << msg.tmi_sent_ts.count()
					<< ' ' << static_cast<commands::CLEARCHAT>(msg);
			}
			template<class Logger>
			Logger& operator<<(Logger& logger, const GLOBALUSERSTATE& msg) {
				return logger
					<< "@badges="      << msg.badges       << ';'
					<< "color="        << msg.color        << ';'
					<< "display-name=" << msg.display_name << ';'
					<< "emote-sets="   << msg.emote_set    << ';'
					<< "user-id="      << msg.user_id      << ';'
					<< "user-type="    << msg.user_type
					<< " :tmi.twitch.tv GLOBALUSERSTATE";
			}
			template<class Logger>
			Logger& operator<<(Logger& logger, const PRIVMSG& msg) {
				logger
					<< "@badges=" << msg.badges << ';'
					<< "color="   << msg.color  << ';';
				if (msg.is_bitsmsg()) {
					logger << "bits=" << msg.bits << ';';
				}
				logger
					<< "display-name=" << msg.display_name        << ';'
					<< "emotes="       << msg.emotes              << ';'
					<< "id="           << msg.id                  << ';'
					<< "mod="          << msg.mod                 << ';'
					<< "room-id="      << msg.room_id             << ';'
					<< "subscriber="   << msg.subscriber          << ';'
					<< "tmi-sent-ts="  << msg.tmi_sent_ts.count() << ';'
					<< "turbo="        << msg.turbo               << ';'
					<< "user-id="      << msg.user_id             << ';'
					<< "user-type="    << msg.user_type
					<< ' ' << static_cast<message::PRIVMSG>(msg);
				return logger; // prevets Error	C2440 'return': cannot convert from 'std::ostream' to 'std::ofstream &'
			}
			template<class Logger>
			Logger& operator<<(Logger& logger, const ROOMSTATE& msg) {
				if (!msg.is_update()) {
					logger
						<< "@broadcaster-lang=" << msg.broadcaster_lang.value_or("") << ';'
						<< "emote-only="        << msg.emote_only.value()       << ';'
						<< "followers-only="    << msg.followers_only.value()   << ';'
						<< "r9k="               << msg.r9k.value()              << ';'
						<< "rituals="           << msg.rituals.value()          << ';'
						<< "room-id="           << msg.room_id                  << ';'
						<< "slow="              << msg.slow.value().count()     << ';'
						<< "subs-only="         << msg.subs_only.value()        << ';';
				}
				else if (msg.emote_only) {
					logger
						<< "@emote-only=" << msg.emote_only.value() << ';'
						<< "room-id="     << msg.room_id;
				}
				else if (msg.followers_only) {
					logger
						<< "@followers-only=" << msg.followers_only.value() << ';'
						<< "room-id="         << msg.room_id;
				}
				else if (msg.r9k) {
					logger
						<< "@r9k="    << msg.r9k.value() << ';'
						<< "room-id=" << msg.room_id;
				}
				else if (msg.slow) {
					logger
						<< "@room-id=" << msg.room_id << ';'
						<< "slow="     << msg.slow.value().count();
				}
				else if (msg.subs_only) {
					logger
						<< "@room-id="  << msg.room_id << ';'
						<< "subs-only=" << msg.subs_only.value();
				}
				else {
					logger << "ROOMSTATE serialization error";
				}
				logger << ' ' << static_cast<commands::ROOMSTATE>(msg);
				return logger; // prevents Error C2440 'return': cannot convert from 'std::ostream' to 'std::ofstream &'

			}

			// USERNOTICE msg-id details
			template<class Logger>
			Logger& operator<<(Logger& logger, const USERNOTICE::ParseError& msg) {
				return logger << "msg-id=ParseError:" << msg.what() << ';';
			}
			template<class Logger>
			Logger& operator<<(Logger& logger, const USERNOTICE::Sub& msg) {
				return logger
					<< (msg.months == 1 ? "msg-id=sub;" : "msg-id=resub;")
					<< "msg-param-months="        << msg.months        << ';'
					<< "msg-param-sub-plan-name=" << msg.sub_plan_name << ';'
					<< "msg-param-sub-plan="      << msg.sub_plan      << ';';
			}
			template<class Logger>
			Logger& operator<<(Logger& logger, const USERNOTICE::Subgift& msg) {
				return logger
					<< "msg-id=subgift;"
					<< "msg-param-months="                 << msg.months                 << ';'
					<< "msg-param-recipient-display-name=" << msg.recipient_display_name << ';'
					<< "msg-param-recipient-id="           << msg.recipient_id           << ';'
					<< "msg-param-recipient-name="         << msg.recipient_name         << ';'
					<< "msg-param-sub-plan-name="          << msg.sub_plan_name          << ';'
					<< "msg-param-sub-plan="               << msg.sub_plan               << ';';

			}
			template<class Logger>
			Logger& operator<<(Logger& logger, const USERNOTICE::Raid& msg) {
				return logger
					<< "msg-id=raid;"
					<< "msg-param-displayName=" << msg.display_name << ';'
					<< "msg-param-login="       << msg.login        << ';'
					<< "msg-param-viewerCount=" << msg.viewer_count << ';';
			}
			template<class Logger>
			Logger& operator<<(Logger& logger, const USERNOTICE::Ritual& msg) {
				return logger << "msg-id=ritual;msg-param-ritual-name=new_chatter;";
			}
			//

			template<class Logger>
			Logger& operator<<(Logger& logger, const USERNOTICE& msg) {
				return logger
					<< "@badges="      << msg.badges       << ';'
					<< "color="        << msg.color        << ';'
					<< "display-name=" << msg.display_name << ';'
					<< "emotes="       << msg.emotes       << ';'
					<< "login="        << msg.login        << ';'
					<< "mod="          << msg.mod          << ';'

					<< msg.msg_id

					<< "room-id="     << msg.room_id             << ';'
					<< "subscriber="  << msg.subscriber          << ';'
					<< "system-msg="  << msg.system_msg          << ';'
					<< "tmi-sent-ts=" << msg.tmi_sent_ts.count() << ';'
					<< "turbo="       << msg.turbo               << ';'
					<< "user-id="     << msg.user_id             << ';'
					<< "user-type="   << msg.user_type
					<< ' ' << static_cast<commands::USERNOTICE>(msg);
			}
			template<class Logger>
			Logger& operator<<(Logger& logger, const USERSTATE& msg) {
				return logger
					<< "@badges="      << msg.badges       << ';'
					<< "color="        << msg.color        << ';'
					<< "display-name=" << msg.display_name << ';'
					<< "emote-sets="   << msg.emote_sets   << ';'
					<< "mod="          << msg.mod          << ';'
					<< "subscriber="   << msg.subscriber   << ';'
					<< "user-type="    << msg.user_type
					<< ' ' << static_cast<commands::USERSTATE>(msg);
			}
		} // namespace tags
	} // namespace cap

	struct ParseError {
		const std::string err;

		inline std::string what() const noexcept { return err; }
	};

	// TODO: remove loggers & initialize Boost.Log to write to file and cout
	template<class Logger>
	struct ParserVisitor : public boost::static_visitor<void>
	{ // TODO: add stats
	private:
		std::string translate_sub_plan(const std::string_view raw_sub_plan) const {
			using namespace std::string_view_literals;
			using namespace std::string_literals;
			if (raw_sub_plan == "Prime"sv) { return "Prime"s ; }
			if (raw_sub_plan == "1000"sv)  { return "Tier 1"s; }
			if (raw_sub_plan == "2000"sv)  { return "Tier 2"s; }
			if (raw_sub_plan == "3000"sv)  { return "Tier 3"s; }
			
			return "Error: sub_plan not handled"s;
		}

		inline void handle_error(const boost::system::error_code& e) const noexcept {
			if (e) {
				Logger::get() << "Error: " << e.message() << '\n';
			}
		}

	public:
		void operator()(const ParseError& e) const {
			Logger::get() << "Parse error: " << e.what() << '\n';
		}
		void operator()(const PING& ping) const {
			using namespace std::string_literals;
			Logger::get() << "PING :"s + ping.host << '\n';
			handle_error(
				m_controller->write("PONG :"s + ping.host)
			);
			BOOST_LOG_TRIVIAL(trace) << ping;
		}
		void operator()(const cap::tags::PRIVMSG& privmsg) const {
			Logger::get() << privmsg << '\n';
			// TODO: add commands
			/*using namespace std::string_literals;
			if (boost::starts_with(privmsg.message, Twitch::irc::Commands::cmd_indicator)) {
				return;
			}*/

			const auto str = privmsg.message.substr(0, privmsg.message.find(' '));
			if (const auto command{ m_commands->find(str) }; command) {
				handle_error(
					m_controller->write(
						"PRIVMSG "s + privmsg.channel + " :" + command(privmsg)
					)
				);
			}
		}

		void operator()(const cap::membership::JOIN& msg) const {
			Logger::get() << "Joins: " << msg.user << '\n';
		}
		void operator()(const cap::membership::PART& msg) const {
			Logger::get() << "Parts: " << msg.user << '\n';
		}
		void operator()(const cap::tags::CLEARCHAT& msg) const {
			if (msg.is_perm()) {
				Logger::get()
					<< msg.user
					<< " has been permanently banned from this channel\n";
			}
			else if (msg.is_clear()) {
				Logger::get()
					<< "Chat has been cleared\n";
			}
			else {
				Logger::get()
					<< msg.user
					<< " has been banned for "
					<< msg.ban_duration.value().count()
					<< "s from this channel\n";
			}
		}
		void operator()(const cap::tags::USERNOTICE& msg) const {
			using namespace std::string_literals;
			const std::string response{ [&] {
				using Sub     = cap::tags::USERNOTICE::Sub;
				using Subgift = cap::tags::USERNOTICE::Subgift;
				using Raid    = cap::tags::USERNOTICE::Raid;

				if (auto* sub_details = boost::get<Sub>(&msg.msg_id); sub_details) {
					return msg.display_name + " has subscribed to this channel"s;
				}
				if (auto* gift_details = boost::get<Subgift>(&msg.msg_id); gift_details) {
					return gift_details->recipient_display_name
						+ " gifted "s + translate_sub_plan(gift_details->sub_plan)
						+ " subscription to "s
						+ gift_details->recipient_name;
				}
				if (auto* raid_details = boost::get<Raid>(&msg.msg_id); raid_details) {
					return raid_details->display_name
						+ " is raiding with a party of "s
						+ std::to_string(raid_details->viewer_count);
				}
				return std::string{};
			}() };

			if (!response.empty()) {
				Logger::get() << response << '\n';
			}
		}
		void operator()(const cap::commands::NOTICE& notice) const {
			Logger::get() << "NOTICE: " << notice.message << '\n';
		}
		void operator()(const cap::tags::USERSTATE& state) const {
			//Logger::get() << state << '\n';
		}
		void operator()(const cap::membership::MODE& msg) const {
			if (msg.gained) {
				Logger::get() << msg.user << " is now a moderator\n";
			}
			else {
				Logger::get() << msg.user << " is no longer a moderator\n";
			}
		}
		void operator()(const cap::commands::HOSTTARGET& host) const {
			if (host.starts()) {
				Logger::get()
					<< "This channel is now hosting "
					<< host.target_channel
					<< '\n';
			}
			else {
				Logger::get()
					<< "This channel is no longer hosting\n";
			}
		}
		void operator()(const cap::tags::GLOBALUSERSTATE& state) const {
			//Logger::get() << state << '\n';
		}
		void operator()(const cap::tags::ROOMSTATE& roomstate) const {
			if (roomstate.is_update()) {
				if (roomstate.emote_only) {
					Logger::get()
						<< (roomstate.emote_only.value()
						? "Channel is no longer in emote only mode\n"
						: "Channel is now in emote only mode\n");
				}
				else if (roomstate.followers_only) {
					if (roomstate.followers_only.value() == -1) {
						Logger::get()
							<< "Channel is no longer in followers only mode\n";
					}
					else {
						Logger::get()
							<< "Channel is now in "
							<< roomstate.followers_only.value()
							<< "t followers only mode\n";
					}
				}
				else if (roomstate.r9k) {
					Logger::get()
						<< (roomstate.r9k.value()
						? "Channel is no longer in r9k mode\n"
						: "Channel is now in r9k mode\n");
				}
				else if (roomstate.rituals) {
					Logger::get() << roomstate.rituals.value() << '\n';
				}
				else if (roomstate.slow) {
					using namespace std::chrono_literals;
					if (roomstate.slow.value() == 0s) {
						Logger::get()
							<< "Channel is no longer in slow mode\n";
					}
					else {
						Logger::get()
							<< "Channel is now in "
							<< roomstate.slow.value().count()
							<< "s slow mode\n";
					}
				}
				else if (roomstate.subs_only) {
					Logger::get()
						<< (roomstate.subs_only.value()
						? "Channel is no longer in sub only mode\n"
						: "Channel is now in sub only mode\n");
				}
			}
			else {
				Logger::get() << roomstate << '\n';
			}
		}
		void operator()(const cap::membership::NAMES& list) const {
			if (list.is_end_of_list()) {
				Logger::get() << "End of /NAMES list\n";
			}
			else {
				auto logger = std::move(Logger::get() << list.channel << " viewers: ");
				if (!list.names.empty()) {
					const auto first = std::begin(list.names);
					logger << *first;
					for (auto it = std::next(first); it != std::end(list.names); ++it) {
						logger << ' ' << *it;
					}
				}
				logger << '\n';
			}
		}
		void operator()([[maybe_unused]] const cap::commands::RECONNECT& r) const {
			Logger::get() << "Reconnecting...\n";
			
			while (const error_code_t error = m_controller->reconnect()) {
				Logger::get() << error.message() << '\n';
			}
		}

		ParserVisitor(
			Twitch::irc::IController* t_controller,
			Twitch::irc::Commands* t_commands
		) :
			m_controller(t_controller),
			m_commands(t_commands)
		{}

	private:
		Twitch::irc::IController* m_controller;
		Twitch::irc::Commands* m_commands;
	};

	// for all caps
	class MessageParser
	{
	public:
		using result_t = boost::variant<
			ParseError,
			PING,
			cap::commands::HOSTTARGET,
			cap::commands::NOTICE,
			cap::commands::RECONNECT,
			cap::tags::CLEARCHAT,
			cap::tags::GLOBALUSERSTATE,
			cap::tags::PRIVMSG,
			cap::tags::ROOMSTATE,
			cap::tags::USERNOTICE,
			cap::tags::USERSTATE,
			cap::membership::JOIN,
			cap::membership::MODE,
			cap::membership::NAMES,
			cap::membership::PART
		>;

		result_t process(std::string_view recived_message);

		template<class TLogger = Logger::DefaultLogger>
		inline ParserVisitor<TLogger> get_visitor(Commands* t_commands) {
			return { m_controller.get(), t_commands };
		}

		MessageParser(std::shared_ptr<IController> irc_controller);

	private:
		std::shared_ptr<IController> m_controller;
	};

} // namespace Twitch::irc::message
#endif