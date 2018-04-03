#ifndef TWITCHMESSAGE_H
#define TWITCHMESSAGE_H

#include <boost\variant.hpp>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <ctime>

class ThreadSafeLogger;

namespace Twitch {
	namespace IRC {
		struct IRCWriter;
		struct IController;
		class Controller;
	} // namespace IRC
} // namespace Twitch

namespace Twitch::IRC::Message {
	struct PING
	{
		std::string host;
	};

	// plain message, without twitch tags
	struct Plain_message
	{
		const std::string nick;
		const std::string user;
		const std::string host;
		const std::string target;
		const std::string message;
	};

	/// https://dev.twitch.tv/docs/irc#twitch-specific-irc-capabilities
	namespace Cap {
		/// https://dev.twitch.tv/docs/irc#twitch-irc-capability-membership
		namespace Membership {
		} // namespace Membership

		/// https://dev.twitch.tv/docs/irc#twitch-irc-capability-tags
		namespace Tags {
			using badge_level = int;
			enum class Badge {
				bits, turbo, subscriber, moderator, broadcaster, global_mod, admin, staff,
				unhandled_badge
			};

			enum class UserType {
				empty, // default
				mod, global_mod, admin, staff,
				unhandled_type
			};

			struct PRIVMSG
			{
				struct Tags
				{ // order is not arbitrary
					const std::vector<std::pair<Badge, badge_level>> badges;
					const std::string color; // #RRGGBB
					const std::string display_name;
					const std::string emotes; // list of emotes and their pos in message, left unprocessed
					const std::string id;
					const bool mod;
					const std::string room_id;
					const bool subscriber;
					const std::time_t tmi_sent_ts;
					const bool turbo;
					const std::string user_id;
					const UserType user_type;
				};

				const Tags tags;
				const Plain_message plain;
			};

			struct BITS
			{
				struct Tags
				{ // order is not arbitrary
					const std::vector<std::pair<Badge, badge_level>> badges;
					const unsigned int bits;
					const std::string color; // #RRGGBB
					const std::string display_name;
					const std::string emotes; // list of emotes and its pos in message, left unprocessed
					const std::string id;
					const bool mod;
					const std::string room_id;
					const bool subscriber;
					const std::time_t tmi_sent_ts;
					const bool turbo;
					const std::string user_id;
					const UserType user_type;
				};

				const Tags tags;
				const Plain_message plain;
			};
		} // namespace Tags
	} // namespace Cap

	namespace MessageParserHelpers {
		std::optional<PING>	              is_ping_message(std::string_view raw_message);
		std::optional<Cap::Tags::PRIVMSG> is_privmsg_message(std::string_view raw_message);
		std::optional<Cap::Tags::BITS>    is_bits_message(std::string_view raw_message);

		std::optional<Plain_message> is_plain_privmsg_message(std::string_view raw_message);
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
			std::cerr << '\n' << e.what() << '\n';
		}
		result_t operator()(const PING& ping) const {
			using namespace std::string_literals;
			Logger::get() << "PING :"s + ping.host << '\n';
			if (auto error = m_ircwriter->write("PONG :"s + ping.host); error) {
				Logger::get() << error.message() << '\n';
			}
		}
		result_t operator()(const Cap::Tags::PRIVMSG& privmsg) const {
			Logger::get() << "PRIVMSG: " << privmsg << '\n';
		}
		result_t operator()(const Cap::Tags::BITS& bits) const {
			Logger::get() << "BITS: " << bits << '\n';
		}
		result_t operator()(const Plain_message& plain) const {
			Logger::get() << "Plain message: " << plain << '\n';
		}

		ParserVisitor(
			Twitch::IRC::IRCWriter* t_writer
		)
			: m_ircwriter(t_writer)
		{}

	private:
		Twitch::IRC::IRCWriter* m_ircwriter;
	};

	class MessageParser
	{
	public:
		using result_t = boost::variant<
			ParseError, PING, Plain_message, Cap::Tags::PRIVMSG, Cap::Tags::BITS
		>;

		result_t process(std::string_view recived_message);

		template<class Logger>
		ParserVisitor<Logger> get_visitor() {
			return { m_controller.get() };
		}

		explicit MessageParser(std::shared_ptr<IController> irc_controller);

	private:
		std::shared_ptr<IController> m_controller;
	};
} // namespace Message

std::ostream& operator<<(std::ostream& stream, const Twitch::IRC::Message::PING& ping);
std::ostream& operator<<(std::ostream& stream, const Twitch::IRC::Message::Plain_message& plainmsg);
std::ostream& operator<<(std::ostream& stream, const Twitch::IRC::Message::Cap::Tags::PRIVMSG& privmsg);
std::ostream& operator<<(std::ostream& stream, const Twitch::IRC::Message::Cap::Tags::BITS& bits);
std::ostream& operator<<(std::ostream& stream, const Twitch::IRC::Message::Cap::Tags::PRIVMSG::Tags& tags);
std::ostream& operator<<(std::ostream& stream, const Twitch::IRC::Message::Cap::Tags::BITS::Tags& tags);
#endif