#include "stdafx.h"
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#define BOOST_TEST_MODULE ParserTest
#include <boost\test\unit_test.hpp>
#include "..\Twitch_C++_IRC_bot\TwitchMessage.h"
#include <array>

/// All copied from official doc page https://dev.twitch.tv/docs/irc
namespace Doc {
	using namespace std::string_literals;
	std::string ping{
		"PING :tmi.twitch.tv"s
	};

	std::string plainmsg{ // from privmsg
		":ronni!ronni@ronni.tmi.twitch.tv PRIVMSG #dallas :Kappa Keepo Kappa"s
	};

	namespace Cap {
		namespace Tags {
			std::string privmsg{
				"@badges=global_mod/1,turbo/1;color=#0D4200;display-name=dallas;emotes=25:0-4,12-16/1902:6-10;id=b34ccfc7-4977-403a-8a94-33c6bac34fb8;mod=0;room-id=1337;subscriber=0;tmi-sent-ts=1507246572675;turbo=1;user-id=1337;user-type=global_mod :ronni!ronni@ronni.tmi.twitch.tv PRIVMSG #dallas :Kappa Keepo Kappa"s
			};
			std::string bitsmsg{
				"@badges=staff/1,bits/1000;bits=100;color=;display-name=dallas;emotes=;id=b34ccfc7-4977-403a-8a94-33c6bac34fb8;mod=0;room-id=1337;subscriber=0;tmi-sent-ts=1507246572675;turbo=1;user-id=1337;user-type=staff :ronni!ronni@ronni.tmi.twitch.tv PRIVMSG #dallas :cheer100"s
			};

			const std::array<std::string, 2> all_cases{
				privmsg, bitsmsg
			};
		}
	}
}

BOOST_AUTO_TEST_SUITE(plainmsg_suite)
	using Twitch::IRC::Message::MessageParserHelpers::is_plain_privmsg_message;
	BOOST_AUTO_TEST_CASE(match_plainmsg)
	{
		BOOST_CHECK(is_plain_privmsg_message(Doc::plainmsg));
	}

	BOOST_AUTO_TEST_CASE(match_tags_messages)
	{
		for(const auto& tcase : Doc::Cap::Tags::all_cases)
			BOOST_CHECK(!is_plain_privmsg_message(tcase));
	}

	BOOST_AUTO_TEST_CASE(match_ping)
	{
		BOOST_CHECK(!is_plain_privmsg_message(Doc::ping));
	}

BOOST_AUTO_TEST_SUITE_END() // plainmsg_suite

BOOST_AUTO_TEST_SUITE(privmsg_suite)
	using Twitch::IRC::Message::MessageParserHelpers::is_privmsg_message;
	BOOST_AUTO_TEST_CASE(match_privmsg)
	{
		BOOST_CHECK(is_privmsg_message(Doc::Cap::Tags::privmsg));
	}

	BOOST_AUTO_TEST_CASE(match_bitsmsg)
	{
		BOOST_CHECK(!is_privmsg_message(Doc::Cap::Tags::bitsmsg));
	}

	BOOST_AUTO_TEST_CASE(match_plain)
	{
		BOOST_CHECK(!is_privmsg_message(Doc::plainmsg));
	}

	BOOST_AUTO_TEST_CASE(match_ping)
	{
		BOOST_CHECK(!is_privmsg_message(Doc::ping));
	}

BOOST_AUTO_TEST_SUITE_END() // privmsg_suite