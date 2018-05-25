#include "stdafx.h"
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#include "IRC_Bot.h"
#include "Logger.h"
#include "TwitchMessage.h"
#include <iostream>
#include <string_view>
#include <fstream>
#include <optional>

namespace {
	bool starts_with(std::string_view str, std::string_view with) {
		if (str.size() < with.size()) { return false; }

		for (size_t i = 0; i < with.size(); ++i) {
			if (str[i] != with[i]) {
				return false;
			}
		}
		return true;
	}

	template<
		class CharT,
		class Traits,
		class Allocator
	> auto& to_lower(std::basic_string<CharT, Traits, Allocator>& str) {
		for (auto& c : str) {
			c = std::tolower(c);
		}
		return str;
	}
	
	struct Config
	{
		inline bool is_good() {
			return
				!server.empty()  &&
			    !port.empty()    &&
				!channel.empty() &&
			    !nick.empty()    &&
			    !token.empty()   &&
				starts_with(channel, "#") &&
				starts_with(token, "oauth:")
			;
		}

		std::string server;
		std::string port;
		std::string channel; // should start with '#'
		std::string nick;	 // all lower case
		std::string token;   // should start with "oauth:"
	};

	std::optional<Config> get_config() {
		Config config;
		std::ifstream file("../config.txt");
		if (!file.is_open()) {
			std::cerr << "Error: config.txt not found\n";
			return std::nullopt;
		}

		std::string temp;
		file >> temp >> temp; config.server  = std::move(temp);
		file >> temp >> temp; config.port    = std::move(temp);
		file >> temp >> temp; config.channel = std::move(temp);
		file >> temp >> temp; config.nick    = std::move(to_lower(temp));
		file >> temp >> temp; config.token   = std::move(temp);
		
		if(!config.is_good()) {
			std::cerr << "Error: config.txt is not properly filled out\n";
			return std::nullopt;
		}
		
		using namespace std::string_view_literals;
		if (!starts_with(config.channel, "#"sv)) {
			std::cerr << "Error: channel have to start with \"#\"\n";
			return std::nullopt;
		}

		if (!starts_with(config.token, "oauth:"sv)) {
			std::cerr << "Error: token must have the prefix \"oauth:\"\n";
			return std::nullopt;
		}

		return config;
	}
}

int main() {
	const auto config = get_config();
	if (!config) {
		return 1;
	}

	Logger::init();

	boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg;

	BOOST_LOG_SEV(lg, boost::log::trivial::severity_level::trace) << "test";
	return 0;
	auto controller{
		std::make_shared<Twitch::irc::Controller>(
			config->server, config->port, config->channel,
			config->nick, config->token,
			std::chrono::milliseconds{ 667 }
		)
	};

	using Twitch::irc::message::cap::tags::PRIVMSG;
	Twitch::irc::TwitchBot bot(
		{
			{
				"!Hello",
				[](const PRIVMSG& msg) {
					return '@' + msg.display_name + " World!";
				}
			}
		},
		controller,
		std::make_unique<Twitch::irc::message::MessageParser>(controller)
	);
	bot.run();
}