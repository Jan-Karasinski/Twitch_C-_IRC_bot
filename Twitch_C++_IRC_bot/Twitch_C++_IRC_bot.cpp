#include "stdafx.h"
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#include "IRC_Bot.h"
#include <iostream>
#include <string_view>
#include <fstream>
#include <optional>

namespace {
	struct Config
	{
		std::string server;
		std::string port;
		std::string channel; // should start with '#'
		std::string nick;	 // all lower case
		std::string token;   // should start with "oauth:"
	};

	std::string& to_lower(std::string& str) {
		for (auto& c : str) {
			c = std::tolower(c);
		}
		return str;
	}

	bool starts_with(std::string_view str, std::string_view with) {
		for (size_t i = 0; i < with.size(); ++i) {
			if (str[i] != with[i])
				return false;
		}
		return true;
	}

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
		
		using namespace std::string_literals;
		if(config.server  == ""s,
		   config.port    == ""s,
		   config.channel == ""s, 
		   config.nick    == ""s,
		   config.token   == ""s)
		{
			std::cerr << "Error: config.txt is empty\n";
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

	auto controller{
		std::make_shared<Twitch::IRC::Controller>(
			config->server, config->port, config->channel,
			config->nick, config->token,
			std::chrono::milliseconds(667)
		)
	};

	Twitch::IRC::TwitchBot bot(
		controller
	);
	bot.run();
}