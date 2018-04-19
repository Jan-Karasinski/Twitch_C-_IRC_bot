#ifndef TWITCHMESSAGE_H
#define TWITCHMESSAGE_H
#include "Logger.h"
#include <boost\variant.hpp>
#include <ctime>
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
		struct IRCWriter;
		struct IController;
		class Controller;
	} // namespace irc
} // namespace Twitch

namespace Twitch::irc::message {
	struct PING
	{
		static const std::regex regex;
		static std::optional<PING> is(std::string_view);

		std::string host;
	};

	// plain message, without twitch tags
	struct Plain_message
	{
		static const std::regex regex;
		static std::optional<Plain_message> is(std::string_view);

		const std::string nick;
		const std::string user;
		const std::string host;
		const std::string target;
		const std::string message;
	};

	/// https://dev.twitch.tv/docs/irc#twitch-specific-irc-capabilities
	namespace cap {
		namespace commands {
			/// raw, extended by cap tags
			struct CLEARCHAT
			{
				static const std::regex regex;
				static std::optional<CLEARCHAT> is(std::string_view);
				
				const std::string channel;
				const std::string user;
			};
			struct HOSTTARGET
			{
				static const std::regex regex;
				static std::optional<HOSTTARGET> is(std::string_view);

				const std::string hosting_channel;
				const std::string target_channel;
				const int viewers_count;
			};
			struct NOTICE
			{
				static const std::regex regex;
				static std::optional<NOTICE> is(std::string_view);

				const std::string msg_id;
				const std::string message;
			};
			struct RECONNECT
			{
				static const std::regex regex;
				static std::optional<RECONNECT> is(std::string_view);
			};
			/// raw, extended by cap tags
			struct ROOMSTATE
			{
				static const std::regex regex;
				static std::optional<ROOMSTATE> is(std::string_view);
				
				const std::string channel;
			};
			/// raw, extended by cap tags
			struct USERNOTICE
			{
				static const std::regex regex;
				static std::optional<USERNOTICE> is(std::string_view);

				const std::string channel;
				const std::string message;
			};
			/// raw, extended by cap tags
			struct USERSTATE
			{
				static const std::regex regex;
				static std::optional<USERSTATE> is(std::string_view);

				const std::string channel;
			};
		} // namespace commands

		/// https://dev.twitch.tv/docs/irc#twitch-irc-capability-membership
		namespace membership {
			struct JOIN
			{
				static const std::regex regex;
				static std::optional<JOIN> is(std::string_view);

				const std::string user;
				const std::string channel;
			};
			struct MODE
			{
				static const std::regex regex;
				static std::optional<MODE> is(std::string_view);

				const std::string channel;
				const bool gained; // true == +o; false == -o
				const std::string user;
			};
			struct NAMES
			{
				static const std::regex regex;
				static std::optional<NAMES> is(std::string_view);
				static const std::string EOL;

				const std::string user;
				const bool end_of_list; // id == 366 == true; id == 353 == false
				const std::string channel;
				const std::vector<std::string> names;
			};
			struct PART
			{
				static const std::regex regex;
				static std::optional<PART> is(std::string_view);

				const std::string user;
				const std::string channel;
			};
		} // namespace membership

		/// https://dev.twitch.tv/docs/irc#twitch-irc-capability-tags
		namespace tags {
			using BadgeLevel = int;
			enum class Badge : int {
				bits, turbo, subscriber, moderator, broadcaster, global_mod, admin, staff,
				unhandled_badge
			};
			enum class UserType {
				empty, // default
				mod, global_mod, admin, staff,
				unhandled_type
			};

			struct CLEARCHAT : public cap::commands::CLEARCHAT
			{
				static const std::regex regex;
				static std::optional<CLEARCHAT> is(std::string_view);

				// default == permanent
				const std::time_t ban_duration{ 0 };
				const std::string ban_reason;

				CLEARCHAT(
					std::time_t&& t_duration,
					std::string&& t_reason,
					std::string&& t_channel,
					std::string&& t_user
				);
			};
			struct GLOBALUSERSTATE
			{
				static const std::regex regex;
				static std::optional<GLOBALUSERSTATE> is(std::string_view);

				const std::string color;
				const std::string display_name;
				const std::string emote_set;
				const bool        turbo;
				const std::string user_id;
				const UserType    user_type;
			};
			struct PRIVMSG : public message::Plain_message
			{
				static const std::regex regex;
				static std::optional<PRIVMSG> is(std::string_view);

				inline bool is_bitsmsg() const noexcept;

				const std::map<Badge, BadgeLevel> badges;
				const unsigned int bits{ 0 }; // default == not bits msg
				const std::string color; // #RRGGBB
				const std::string display_name;
				const std::string emotes; // list of emotes and their pos in message, left unprocessed
				const std::string id;
				const bool        mod;
				const std::string room_id;
				const bool        subscriber;
				const std::time_t tmi_sent_ts;
				const bool        turbo;
				const std::string user_id;
				const UserType    user_type;

				PRIVMSG(
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
				);
			};
			struct ROOMSTATE : cap::commands::ROOMSTATE
			{
				static const std::regex regex;
				static std::optional<ROOMSTATE> is(std::string_view);

				// empty if not enabled
				std::optional<const std::string> broadcaster_lang{ std::nullopt };
				std::optional<const bool>        r9k;
				std::optional<const std::time_t> slow;
				std::optional<const bool>        subs_only;
				
				ROOMSTATE(
					cap::commands::ROOMSTATE&& t_roomstate,
					std::optional<const std::string>&& t_lang,
					std::optional<const bool>&&        t_r9k,
					std::optional<const std::time_t>&& t_slow,
					std::optional<const bool>&&        t_subs_only
				);
			};
			struct USERNOTICE : public cap::commands::USERNOTICE
			{
				struct ParseError {};
				struct Sub
				{
					static const std::regex regex;
					static std::optional<Sub> is(std::string_view);

					const int msg_param_months;
					const std::string msg_param_sub_plan;
					const std::string msg_param_sub_plan_name;
				};
				struct Subgift
				{
					static const std::regex regex;
					static std::optional<Subgift> is(std::string_view);

					const int msg_param_months;
					const std::string msg_param_display_name;
					const std::string msg_param_recipient_id;
					const std::string msg_param_recipient_name;
					const std::string msg_param_sub_plan_name;
					const std::string msg_param_sub_plan;
				};
				struct Raid
				{
					static const std::regex regex;
					static std::optional<Raid> is(std::string_view);

					const std::string msg_param_displayName;
					const std::string msg_param_login;
					const int msg_param_viewerCount;
				};
				struct Ritual
				{
					static const std::regex regex;
					static std::optional<Ritual> is(std::string_view);
				};

				static const std::regex regex;
				static std::optional<USERNOTICE> is(std::string_view);

				const std::map<Badge, BadgeLevel> badges;
				const std::string color;
				const std::string display_name;
				const std::string emotes;
				const std::string id;
				const std::string login;
				const bool        mod;
				const boost::variant<ParseError, Sub, Subgift, Raid, Ritual> msg_id;
				const std::string room_id;
				const bool        subscriber;
				const std::string system_msg;
				const std::time_t tmi_sent_ts;
				const bool        turbo;
				const std::string user_id;
				const UserType    user_type;

				USERNOTICE(
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
				);
			};
			struct USERSTATE : public cap::commands::USERSTATE
			{
				static const std::regex regex;
				static std::optional<USERSTATE> is(std::string_view);

				const std::string color;
				const std::string display_name;
				const std::string emote_sets;
				const bool        mod;
				const bool        subscriber;
				const bool        turbo;
				const UserType    user_type;

				USERSTATE(
					cap::commands::USERSTATE&& t_userstate,
					std::string&& t_color,
					std::string&& t_display_name,
					std::string&& t_emote_sets,
					bool          t_mod,
					bool          t_subscriber,
					bool          t_turbo,
					UserType      t_user_type
				);
			};
		} // namespace tags
	} // namespace cap

	namespace MessageParserHelpers {
		std::optional<PING>	              is_ping_message(std::string_view raw_message);
		std::optional<Plain_message>      is_plain_privmsg_message(std::string_view raw_message);

		std::optional<cap::tags::CLEARCHAT>       is_clearchat(std::string_view raw_message);
		std::optional<cap::tags::GLOBALUSERSTATE> is_globaluserstate(std::string_view raw_message);
		std::optional<cap::tags::PRIVMSG>         is_privmsg_message(std::string_view raw_message);
		//std::optional<cap::tags::BITSMSG>            is_bits_message(std::string_view raw_message);
		std::optional<cap::tags::ROOMSTATE>       is_roomstate(std::string_view raw_message);
		std::optional<cap::tags::USERNOTICE>      is_usernotice(std::string_view raw_message);
		std::optional<cap::tags::USERSTATE>       is_userstate(std::string_view raw_message);

	} // namespace MessageParserHelpers

	struct ParseError {
		const std::string err;

		inline std::string what() const noexcept { return err; }
	};

	// required members for Logger class:
	// static Logger get()
	// ostream& operator<<(T) 
	template<class Logger>
	struct ParserVisitor : public boost::static_visitor<void>
	{
		using result_t = void;

		result_t operator()(const ParseError& e) const {
			Logger::get() << e.what() << '\n';
		}
		result_t operator()(const PING& ping) const {
			using namespace std::string_literals;
			Logger::get() << "PING :"s + ping.host << '\n';
			if (auto error = m_ircwriter->write("PONG :"s + ping.host); error) {
				Logger::get() << error.message() << '\n';
			}
		}
		result_t operator()(const cap::tags::PRIVMSG& privmsg) const {
			//Logger::get() << "PRIVMSG: " << privmsg << '\n';
		}
		/*result_t operator()(const cap::tags::BITSMSG& bits) const {
			Logger::get() << "BITSMSG: " << bits << '\n';
		}*/


		result_t operator()(const Plain_message& plain) const {
			//Logger::get() << "Plain message: " << plain << '\n';
		}

		ParserVisitor(
			Twitch::irc::IRCWriter* t_writer
		)
			: m_ircwriter(t_writer)
		{}

	private:
		Twitch::irc::IRCWriter* m_ircwriter;
	};

	class MessageParser
	{
	public:
		using result_t = boost::variant<
			ParseError, PING, Plain_message,
			cap::tags::PRIVMSG//, cap::tags::BITSMSG
		>;

		result_t process(std::string_view recived_message);

		template<class TLogger = Logger::DefaultLogger>
		ParserVisitor<TLogger> get_visitor() {
			return { m_writer.get() };
		}

		explicit MessageParser(std::shared_ptr<IRCWriter> irc_writer);

	private:
		std::shared_ptr<IRCWriter> m_writer;
	};
} // namespace Twitch::irc::message

std::ostream& operator<<(std::ostream& stream, const Twitch::irc::message::cap::tags::Badge& badge);
std::ostream& operator<<(std::ostream& stream, const Twitch::irc::message::PING& ping);
std::ostream& operator<<(std::ostream& stream, const Twitch::irc::message::Plain_message& plainmsg);
std::ostream& operator<<(std::ostream& stream, const Twitch::irc::message::cap::tags::PRIVMSG& privmsg);
//std::ostream& operator<<(std::ostream& stream, const Twitch::irc::message::cap::tags::BITSMSG& bits);
//std::ostream& operator<<(std::ostream& stream, const Twitch::irc::message::cap::tags::PRIVMSG::Tags& tags);
//std::ostream& operator<<(std::ostream& stream, const Twitch::irc::message::cap::tags::BITSMSG::Tags& tags);
#endif