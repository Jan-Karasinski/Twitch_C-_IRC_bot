#include "stdafx.h"
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#include "TwitchMessage.h"
#include "IRC_Bot.h"
#include <boost\algorithm\string\classification.hpp>
#include <boost\algorithm\string\split.hpp>
#include <iostream>
#include <exception>
#include <regex>
#include <string_view>
#include <sstream>

namespace { // helpers
	using Twitch::IRC::Message::Cap::Tags::Badge;
	using Twitch::IRC::Message::Cap::Tags::UserType;
	using Twitch::IRC::Message::Cap::Tags::badge_level;

	inline Badge string_to_badge(std::string_view badge) {
		using namespace std::string_view_literals;
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
		using namespace std::string_literals;
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
		using namespace std::string_literals;
		if      (utype == UserType::empty)      { return ""s; }
		else if (utype == UserType::mod)        { return "mod"s; }
		else if (utype == UserType::global_mod) { return "global_mod"s; }
		else if (utype == UserType::admin)      { return "admin"s; }
		else if (utype == UserType::staff)      { return "staff"s; }
		else
			return "unhandled_type"s;
	}

	std::vector<std::pair<Badge, badge_level>> get_badges(std::string_view raw_badges) {
		if (raw_badges.size() == 0) { return {}; }

		std::vector<std::string> splitted;
		boost::split(
			splitted,
			raw_badges,
			boost::is_any_of(",/"),
			boost::algorithm::token_compress_on
		);
		if (splitted.size() % 2 != 0) return {};

		std::vector<std::pair<Badge, badge_level>> badges;
		for (std::size_t i{ 0 }; i < splitted.size() / 2; i += 2) {
			badges.emplace_back(
				string_to_badge(splitted[i]),
				std::stoi(splitted[i + 1])
			);
		}
		return badges;
	}

	inline bool get_mod(std::string_view raw_message) noexcept {
		using namespace std::string_view_literals;
		return raw_message == "1"sv;
	}

	inline bool get_subscriber(std::string_view raw_message) noexcept {
		using namespace std::string_view_literals; 
		return raw_message == "1"sv;
	}

	std::time_t get_tmi_sent_ts(const std::string& raw_message) {
		std::stringstream stream{ raw_message };
		std::time_t ts;
		stream >> ts;
		return ts;
	}

	inline bool get_turbo(std::string_view raw_message) noexcept {
		using namespace std::string_view_literals; 
		return raw_message == "1"sv;
	}

	inline UserType get_user_type(std::string_view raw_message) {
		using namespace std::string_view_literals;
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
}

using Twitch::IRC::Message::PING;
using Twitch::IRC::Message::Plain_message;
using Twitch::IRC::Message::Cap::Tags::PRIVMSG;
using Twitch::IRC::Message::Cap::Tags::BITS;
using Twitch::IRC::Message::Cap::Tags::Badge;

namespace Twitch::IRC::Message {
	namespace MessageParserHelpers {
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
				R"(@badges=(.*);color=(.+);display-name=(.+);emotes=(.*);id=(.+);mod=(.+);room-id=(.+);)"
				R"(subscriber=(.);tmi-sent-ts=(.+);turbo=(.);user-id=(.+);user-type=(.*))"
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

			return PRIVMSG{
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
			};
		}

		std::optional<BITS> is_bits_message(std::string_view raw_message) {
			std::cmatch match;

			static const std::regex bits_regex{
				R"(@badges=(.*);bits(.+);color=(.+);display-name=(.+);emotes=(.*);id=(.+);mod=(.+);room-id=(.+);)"
				R"(subscriber=(.);tmi-sent-ts=(.+);turbo=(.);user-id=(.+);user-type=(.*))"
				R"( :(.+)!(.+)@(.+) PRIVMSG (#.+) :(.+)[\r\n|\r|\n]?)"
			};
			if (!std::regex_match(raw_message.data(), match, bits_regex)) { return std::nullopt; }

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

			return BITS{
				BITS::Tags{
					get_badges(match.str(badges)),
					std::stoul(match.str(bits)),
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
			};
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
			using MessageParserHelpers::is_bits_message;
			using MessageParserHelpers::is_ping_message;
			using MessageParserHelpers::is_plain_privmsg_message;
			using MessageParserHelpers::is_privmsg_message;

			if (auto parsed = is_ping_message(recived_message); parsed) {
				return *parsed;
			}
			// Cap tags
			if (auto parsed = is_privmsg_message(recived_message); parsed) {
				return *parsed;
			}
			if (auto parsed = is_bits_message(recived_message); parsed) {
				return *parsed;
			}
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

std::ostream& operator<<(std::ostream& stream, const Badge& badge) {
	if (badge == Badge::subscriber) { return stream << "subscriber"; }
	else if (badge == Badge::bits) { return stream << "bits"; }
	else if (badge == Badge::turbo) { return stream << "turbo"; }
	else if (badge == Badge::moderator) { return stream << "moderator"; }
	else if (badge == Badge::broadcaster) { return stream << "broadcaster"; }
	else if (badge == Badge::global_mod) { return stream << "global_mod"; }
	else if (badge == Badge::admin) { return stream << "admin"; }
	else if (badge == Badge::staff) { return stream << "staff"; }
	else return stream << "unhandled_badge";
}

std::ostream& operator<<(std::ostream& stream, const PING& ping) {
	return stream << "\nPING :" << ping.host << '\n';
}

std::ostream& operator<<(std::ostream& stream, const Plain_message& plainmsg) {
	return stream << ':'
		<< plainmsg.nick << '!'
		<< plainmsg.user << '@'
		<< plainmsg.host
		<< plainmsg.target << " :"
		<< plainmsg.message
		<< '\n';
}

std::ostream& operator<<(std::ostream& stream, const PRIVMSG& privmsg) {
	return stream
		<< privmsg.tags << ' '
		<< privmsg.plain;
}

std::ostream& operator<<(std::ostream& stream, const BITS& bits) {
	return stream
		<< bits.tags << ' '
		<< bits.plain;
}

std::ostream& operator<<(std::ostream& stream, const PRIVMSG::Tags& tags) {
	stream << "@badges=";
	for (auto p : tags.badges) {
		stream << p.first << '/' << p.second;
	}
	return stream
		<< ";color=" << tags.color
		<< ";display-name=" << tags.display_name
		<< ";emotes=" << tags.emotes
		<< ";id=" << tags.id
		<< ";mod=" << tags.mod
		<< ";room-id=" << tags.room_id
		<< ";subscriber=" << tags.subscriber
		<< ";tmi-sent-ts=" << tags.tmi_sent_ts
		<< ";turbo=" << tags.turbo
		<< ";user-id=" << tags.user_id
		<< ";user-type=" << user_type_to_string(tags.user_type);
}

std::ostream& operator<<(std::ostream& stream, const BITS::Tags& tags) {
	stream << "@badges=";
	for (auto p : tags.badges) {
		stream << p.first << '/' << p.second;
	}
	return stream
		<< ";bits=" << tags.bits
		<< ";color=" << tags.color
		<< ";display-name=" << tags.display_name
		<< ";emotes=" << tags.emotes
		<< ";id=" << tags.id
		<< ";mod=" << tags.mod
		<< ";room-id=" << tags.room_id
		<< ";subscriber=" << tags.subscriber
		<< ";tmi-sent-ts=" << tags.tmi_sent_ts
		<< ";turbo=" << tags.turbo
		<< ";user-id=" << tags.user_id
		<< ";user-type=" << user_type_to_string(tags.user_type);
}