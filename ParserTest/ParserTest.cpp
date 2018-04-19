#include "stdafx.h"
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#define BOOST_TEST_MODULE ParserTest
#include <boost\test\unit_test.hpp>
#include "..\Twitch_C++_IRC_bot\TwitchMessage.h"
#include <vector>
#include <functional>
#include <tuple>
#include <type_traits>

namespace {
	using namespace std::string_literals;
	std::string privmsg_from_ordinary_user{
		"@badges=;color=;display-name=feroketis;emotes=;"
		"id=46f9ed11-cfc2-47fd-ac39-66b434ace377;mod=0;"
		"room-id=87056709;subscriber=0;tmi-sent-ts=1523014439808;"
		"turbo=0;user-id=59867170;user-type="
		" :feroketis!feroketis@feroketis.tmi.twitch.tv PRIVMSG #pgl_dota2 :!vote sccc"s
	};
}
/// All copied from official doc page https://dev.twitch.tv/docs/irc
namespace doc {
	using namespace std::string_literals;
	std::string ping{
		"PING :tmi.twitch.tv"s
	};
	std::string plainmsg{ // from privmsg
		":ronni!ronni@ronni.tmi.twitch.tv PRIVMSG #dallas :Kappa Keepo Kappa"s
	};

	namespace cap {
		namespace membership {
			namespace join {
				const std::string msg1{
					":ronni!ronni@ronni.tmi.twitch.tv JOIN #dallas"s
				};

				const std::vector<const std::string*> all_cases{
					&msg1
				};
			}
			namespace mode {
				const std::string msg1{
					":jtv MODE #dallas +o ronni"s
				};
				const std::string msg2{
					":jtv MODE #dallas -o ronni"s
				};

				const std::vector<const std::string*> all_cases{
					&msg1, &msg2
				};
			}
			namespace names {
				const std::string msg1{
					":ronni.tmi.twitch.tv 353 ronni = #dallas : ronni fred wilma"s
				};
				const std::string msg2{
					":ronni.tmi.twitch.tv 353 ronni = #dallas :barney betty"s
				};
				const std::string msg3{
					":ronni.tmi.twitch.tv 366 ronni #dallas :End of /NAMES list"s
				};

				const std::vector<const std::string*> all_cases{
					&msg1, &msg2, &msg3
				};
			}
			namespace part {
				const std::string msg1{
					":ronni!ronni@ronni.tmi.twitch.tv PART #dallas"s
				};

				const std::vector<const std::string*> all_cases{
					&msg1
				};
			}
		}
		namespace tags {
			namespace clearchat {
				const std::string msg1{
					R"(@ban-reason=Follow\sthe\srules :tmi.twitch.tv CLEARCHAT #dallas :ronni)"s
				};

				const std::vector<const std::string*> all_cases{
					&msg1
				};
			}
			namespace globaluserstate {
				const std::string msg1{
					"@color=#0D4200;display-name=dallas;"
					"emote-sets=0,33,50,237,793,2126,3517,4578,5569,9400,10337,12239;"
					"turbo=0;user-id=1337;user-type=admin :tmi.twitch.tv GLOBALUSERSTATE"s
				};

				const std::vector<const std::string*> all_cases{
					&msg1
				};
			}
			namespace privmsg {
				const std::string msg1{
					"@badges=global_mod/1,turbo/1;color=#0D4200;"
					"display-name=dallas;emotes=25:0-4,12-16/1902:6-10;"
					"id=b34ccfc7-4977-403a-8a94-33c6bac34fb8;mod=0;"
					"room-id=1337;subscriber=0;tmi-sent-ts=1507246572675;"
					"turbo=1;user-id=1337;user-type=global_mod"
					" :ronni!ronni@ronni.tmi.twitch.tv PRIVMSG #dallas :Kappa Keepo Kappa"s
				};
				const std::string msg2{
					"@badges=staff/1,bits/1000;bits=100;color=;display-name=dallas;"
					"emotes=;id=b34ccfc7-4977-403a-8a94-33c6bac34fb8;mod=0;"
					"room-id=1337;subscriber=0;tmi-sent-ts=1507246572675;"
					"turbo=1;user-id=1337;user-type=staff"
					" :ronni!ronni@ronni.tmi.twitch.tv PRIVMSG #dallas :cheer100"s
				};

				const std::vector<const std::string*> all_cases{
					&msg1, &msg2
				};
			}
			namespace roomstate {
				const std::string msg1{
					"@broadcaster-lang=en;r9k=0;slow=0;subs-only=0"
					" :tmi.twitch.tv ROOMSTATE #dallas"s
				};
				const std::string msg2{ // change of state
					"@slow=10 :tmi.twitch.tv ROOMSTATE #dallas"s
				};

				const std::vector<const std::string*> all_cases{
					&msg1, &msg2
				};
			}
			namespace usernotice {
				const std::string msg1{ // resub
					"@badges=staff/1,broadcaster/1,turbo/1;color=#008000;display-name=ronni;"
					"emotes=;id=db25007f-7a18-43eb-9379-80131e44d633;login=ronni;"
					"mod=0;msg-id=resub;msg-param-months=6;msg-param-sub-plan=Prime;"
					"msg-param-sub-plan-name=Prime;room-id=1337;subscriber=1;"
					R"(system-msg=ronni\shas\ssubscribed\sfor\s6\smonths!;)"
					"tmi-sent-ts=1507246572675;turbo=1;user-id=1337;user-type=staff"
					" :tmi.twitch.tv USERNOTICE #dallas :Great stream -- keep it up!"s
				};
				const std::string msg2{ // subgift
					"@badges=staff/1,premium/1;color=#0000FF;display-name=TWW2;"
					"emotes=;id=e9176cd8-5e22-4684-ad40-ce53c2561c5e;login=tww2;"
					"mod=0;msg-id=subgift;msg-param-months=1;"
					"msg-param-recipient-display-name=Mr_Woodchuck;"
					"msg-param-recipient-id=89614178;msg-param-recipient-name=mr_woodchuck;"
					R"(msg-param-sub-plan-name=House\sof\sNyoro~n;msg-param-sub-plan=1000;)"
					"room-id=19571752;subscriber=0;"
					R"(system-msg=TWW2\sgifted\sa\sTier\s1\ssub\sto\sMr_Woodchuck!;)"
					"tmi-sent-ts=1521159445153;turbo=0;user-id=13405587;"
					"user-type=staff :tmi.twitch.tv USERNOTICE #forstycup"s
				};
				const std::string msg3{ // raid
					"@badges=turbo/1;color=#9ACD32;display-name=TestChannel;emotes=;"
					"id=3d830f12-795c-447d-af3c-ea05e40fbddb;login=testchannel;mod=0;"
					"msg-id=raid;msg-param-displayName=TestChannel;msg-param-login=testchannel;"
					"msg-param-viewerCount=15;room-id=56379257;subscriber=0;"
					R"(system-msg=15\sraiders\sfrom\sTestChannel\shave\sjoined\n!;)"
					"tmi-sent-ts=1507246572675;tmi-sent-ts=1507246572675;turbo=1;"
					"user-id=123456;user-type= :tmi.twitch.tv USERNOTICE #othertestchannel"s
				};
				const std::string msg4{ // ritual
					"@badges=;color=;display-name=SevenTest1;emotes=30259:0-6;"
					"id=37feed0f-b9c7-4c3a-b475-21c6c6d21c3d;login=seventest1;"
					"mod=0;msg-id=ritual;msg-param-ritual-name=new_chatter;"
					R"(room-id=6316121;subscriber=0;system-msg=Seventoes\sis\snew\shere!;)"
					"tmi-sent-ts=1508363903826;turbo=0;user-id=131260580;"
					"user-type= :tmi.twitch.tv USERNOTICE #seventoes :HeyGuys"s
				};

				const std::vector<const std::string*> all_cases{
					&msg1, &msg2, &msg3, &msg4
				};
			}
			namespace userstate {
				const std::string msg1{
					"@color=#0D4200;display-name=ronni;"
					"emote-sets=0,33,50,237,793,2126,3517,4578,5569,9400,10337,12239;"
					"mod=1;subscriber=1;turbo=1;user-type=staff"
					" :tmi.twitch.tv USERSTATE #dallas"s
				};

				const std::vector<const std::string*> all_cases{
					&msg1
				};
			}
		}
		namespace commands {
			namespace clearchat {
				const std::string msg1{
					":tmi.twitch.tv CLEARCHAT #dallas"s
				};
				const std::string msg2{
					":tmi.twitch.tv CLEARCHAT #<channel> :<user>"s
				};
				
				const std::vector<const std::string*> all_cases{
					&msg1, &msg2
				};
			}
			namespace hosttarget {
				const std::string msg1{
					":tmi.twitch.tv HOSTTARGET #hosting_channel <channel> [0]"s
				};
				const std::string msg2{
					":tmi.twitch.tv HOSTTARGET #hosting_channel :- [0]"s
				};

				const std::vector<const std::string*> all_cases{
					&msg1, &msg2
				};
			}
			namespace notice {
				const std::string msg1{
					"@msg-id=slow_off :tmi.twitch.tv NOTICE"
					" #dallas :This room is no longer in slow mode."s
				};
				
				const std::vector<const std::string*> all_cases{
					&msg1
				};
			}
			namespace reconnect {
				const std::string msg1{
					"RECONNECT"s
				};
				
				const std::vector<const std::string*> all_cases{
					&msg1
				};
			}
			namespace roomstate {
				const std::string msg1{
					":tmi.twitch.tv ROOMSTATE #<channel>"s
				};
				
				const std::vector<const std::string*> all_cases{
					&msg1
				};
			}
			namespace usernotice {
				const std::string msg1{
					":tmi.twitch.tv USERNOTICE #<channel> :message"s
				};
				
				const std::vector<const std::string*> all_cases{
					&msg1
				};
			}
			namespace userstate {
				const std::string msg1{
					":tmi.twitch.tv USERSTATE #<channel>"s
				};
				
				const std::vector<const std::string*> all_cases{
					&msg1
				};
			}
		}
	}
}

#define MATCH_RAW(MessageType)\
	namespace message = Twitch::irc::message;\
	BOOST_AUTO_TEST_CASE(match_PING)\
	{\
		BOOST_CHECK((static_cast<bool>(MessageType::is(::doc::ping)) == std::is_same<MessageType, message::PING>::value));\
	}\
	BOOST_AUTO_TEST_CASE(match_PLAINMSG)\
	{\
		BOOST_CHECK((static_cast<bool>(MessageType::is(::doc::plainmsg)) == std::is_same<MessageType, message::Plain_message>::value));\
	}

#define MATCH_COMMANDS(MessageType)\
	namespace commands = Twitch::irc::message::cap::commands;\
	BOOST_AUTO_TEST_CASE(match_CLEARCHAT)\
	{\
		for (const auto* msg : doc::cap::commands::clearchat::all_cases) {\
			BOOST_CHECK((static_cast<bool>(MessageType::is(*msg)) == std::is_same<MessageType, commands::CLEARCHAT>()));\
		}\
	}\
	BOOST_AUTO_TEST_CASE(match_HOSTTARGET)\
	{\
		for (const auto* msg : doc::cap::commands::hosttarget::all_cases) {\
			BOOST_CHECK((static_cast<bool>(MessageType::is(*msg)) == std::is_same<MessageType, commands::HOSTTARGET>()));\
		}\
	}\
	BOOST_AUTO_TEST_CASE(match_NOTICE)\
	{\
		for (const auto* msg : doc::cap::commands::notice::all_cases) {\
			BOOST_CHECK((static_cast<bool>(MessageType::is(*msg)) == std::is_same<MessageType, commands::NOTICE>()));\
		}\
	}\
	BOOST_AUTO_TEST_CASE(match_RECONNECT)\
	{\
		for (const auto* msg : doc::cap::commands::reconnect::all_cases) {\
			BOOST_CHECK((static_cast<bool>(MessageType::is(*msg)) == std::is_same<MessageType, commands::RECONNECT>()));\
		}\
	}\
	BOOST_AUTO_TEST_CASE(match_ROOMSTATE)\
	{\
		for (const auto* msg : doc::cap::commands::roomstate::all_cases) {\
			BOOST_CHECK((static_cast<bool>(MessageType::is(*msg)) == std::is_same<MessageType, commands::ROOMSTATE>()));\
		}\
	}\
	BOOST_AUTO_TEST_CASE(match_USERNOTICE)\
	{\
		for (const auto* msg : doc::cap::commands::usernotice::all_cases) {\
			BOOST_CHECK((static_cast<bool>(MessageType::is(*msg)) == std::is_same<MessageType, commands::USERNOTICE>()));\
		}\
	}\
	BOOST_AUTO_TEST_CASE(match_USERSTATE)\
	{\
		for (const auto* msg : doc::cap::commands::userstate::all_cases) {\
			BOOST_CHECK((static_cast<bool>(MessageType::is(*msg)) == std::is_same<MessageType, commands::USERSTATE>()));\
		}\
	}

#define MATCH_MEMBERSHIP(MessageType)\
	namespace membership = Twitch::irc::message::cap::membership;\
	BOOST_AUTO_TEST_CASE(match_JOIN)\
	{\
		for (const auto* msg : doc::cap::membership::join::all_cases) {\
			BOOST_CHECK((static_cast<bool>(MessageType::is(*msg)) == std::is_same<MessageType, membership::JOIN>()));\
		}\
	}\
	BOOST_AUTO_TEST_CASE(match_MODE)\
	{\
		for (const auto* msg : doc::cap::membership::mode::all_cases) {\
			BOOST_CHECK((static_cast<bool>(MessageType::is(*msg)) == std::is_same<MessageType, membership::MODE>()));\
		}\
	}\
	BOOST_AUTO_TEST_CASE(match_NAMES)\
	{\
		for (const auto* msg : doc::cap::membership::names::all_cases) {\
			BOOST_CHECK((static_cast<bool>(MessageType::is(*msg)) == std::is_same<MessageType, membership::NAMES>()));\
		}\
	}\
	BOOST_AUTO_TEST_CASE(match_PART)\
	{\
		for (const auto* msg : doc::cap::membership::part::all_cases) {\
			BOOST_CHECK((static_cast<bool>(MessageType::is(*msg)) == std::is_same<MessageType, membership::PART>()));\
		}\
	}

#define MATCH_TAGS(MessageType)\
	namespace tags = Twitch::irc::message::cap::tags;\
	BOOST_AUTO_TEST_CASE(match_CLEARCHAT)\
	{\
		for (const auto* msg : doc::cap::tags::clearchat::all_cases) {\
			BOOST_CHECK((static_cast<bool>(MessageType::is(*msg)) == std::is_same<MessageType, tags::CLEARCHAT>()));\
		}\
	}\
	BOOST_AUTO_TEST_CASE(match_GLOBALUSERSTATE)\
	{\
		for (const auto* msg : doc::cap::tags::globaluserstate::all_cases) {\
			BOOST_CHECK((static_cast<bool>(MessageType::is(*msg)) == std::is_same<MessageType, tags::GLOBALUSERSTATE>()));\
		}\
	}\
	BOOST_AUTO_TEST_CASE(match_PRIVMSG)\
	{\
		for (const auto* msg : doc::cap::tags::privmsg::all_cases) {\
			BOOST_CHECK((static_cast<bool>(MessageType::is(*msg)) == std::is_same<MessageType, tags::PRIVMSG>()));\
		}\
	}\
	BOOST_AUTO_TEST_CASE(match_ROOMSTATE)\
	{\
		for (const auto* msg : doc::cap::tags::roomstate::all_cases) {\
			BOOST_CHECK((static_cast<bool>(MessageType::is(*msg)) == std::is_same<MessageType, tags::ROOMSTATE>()));\
		}\
	}\
	BOOST_AUTO_TEST_CASE(match_USERNOTICE)\
	{\
		for (const auto* msg : doc::cap::tags::usernotice::all_cases) {\
			BOOST_CHECK((static_cast<bool>(MessageType::is(*msg)) == std::is_same<MessageType, tags::USERNOTICE>()));\
		}\
	}\
	BOOST_AUTO_TEST_CASE(match_USERSTATE)\
	{\
		for (const auto* msg : doc::cap::tags::userstate::all_cases) {\
			BOOST_CHECK((static_cast<bool>(MessageType::is(*msg)) == std::is_same<MessageType, tags::USERSTATE>()));\
		}\
	}

#define MATCH_RAW_SUITE_TEMPLATE(MessageType)\
	BOOST_AUTO_TEST_SUITE(match_raw)\
		MATCH_RAW(MessageType)\
	BOOST_AUTO_TEST_SUITE_END()

#define MATCH_COMMANDS_SUITE_TEMPLATE(MessageType)\
	BOOST_AUTO_TEST_SUITE(match_commands)\
		MATCH_COMMANDS(MessageType)\
	BOOST_AUTO_TEST_SUITE_END()

#define MATCH_MEMBERSHIP_SUITE_TEMPLATE(MessageType)\
	BOOST_AUTO_TEST_SUITE(match_membership)\
		MATCH_MEMBERSHIP(MessageType)\
	BOOST_AUTO_TEST_SUITE_END()

#define MATCH_TAGS_SUITE_TEMPLATE(MessageType)\
	BOOST_AUTO_TEST_SUITE(match_tags)\
		MATCH_TAGS(MessageType)\
	BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(raw_messages_tests_suite)
	BOOST_AUTO_TEST_SUITE(PING_suite)
		using Twitch::irc::message::PING;

		MATCH_RAW_SUITE_TEMPLATE       (PING)
		MATCH_COMMANDS_SUITE_TEMPLATE  (PING)
		MATCH_MEMBERSHIP_SUITE_TEMPLATE(PING)
		MATCH_TAGS_SUITE_TEMPLATE      (PING)
	BOOST_AUTO_TEST_SUITE_END() // PING_suite

	BOOST_AUTO_TEST_SUITE(PLAINMSG_suite)
		using Twitch::irc::message::Plain_message;
	
		MATCH_RAW_SUITE_TEMPLATE       (Plain_message)
		MATCH_COMMANDS_SUITE_TEMPLATE  (Plain_message)
		MATCH_MEMBERSHIP_SUITE_TEMPLATE(Plain_message)
		MATCH_TAGS_SUITE_TEMPLATE      (Plain_message)
	BOOST_AUTO_TEST_SUITE_END() // PLAINMSG_suite
BOOST_AUTO_TEST_SUITE_END() // raw_messages_tests_suite

BOOST_AUTO_TEST_SUITE(commands_tests_suite)
	BOOST_AUTO_TEST_SUITE(CLEARCHAT_suite)
		using Twitch::irc::message::cap::commands::CLEARCHAT;

		MATCH_RAW_SUITE_TEMPLATE       (CLEARCHAT)
		MATCH_COMMANDS_SUITE_TEMPLATE  (CLEARCHAT)
		MATCH_MEMBERSHIP_SUITE_TEMPLATE(CLEARCHAT)
		MATCH_TAGS_SUITE_TEMPLATE      (CLEARCHAT)
	BOOST_AUTO_TEST_SUITE_END() // CLEARCHAT_suite

	BOOST_AUTO_TEST_SUITE(HOSTTARGET_suite)
		using Twitch::irc::message::cap::commands::HOSTTARGET;

		MATCH_RAW_SUITE_TEMPLATE       (HOSTTARGET)
		MATCH_COMMANDS_SUITE_TEMPLATE  (HOSTTARGET)
		MATCH_MEMBERSHIP_SUITE_TEMPLATE(HOSTTARGET)
		MATCH_TAGS_SUITE_TEMPLATE      (HOSTTARGET)
	BOOST_AUTO_TEST_SUITE_END() // HOSTTARGET_suite

	BOOST_AUTO_TEST_SUITE(NOTICE_suite)
		using Twitch::irc::message::cap::commands::NOTICE;

		MATCH_RAW_SUITE_TEMPLATE       (NOTICE)
		MATCH_COMMANDS_SUITE_TEMPLATE  (NOTICE)
		MATCH_MEMBERSHIP_SUITE_TEMPLATE(NOTICE)
		MATCH_TAGS_SUITE_TEMPLATE      (NOTICE)
	BOOST_AUTO_TEST_SUITE_END() // NOTICE_suite

	BOOST_AUTO_TEST_SUITE(RECONNECT_suite)
		using Twitch::irc::message::cap::commands::RECONNECT;

		MATCH_RAW_SUITE_TEMPLATE       (RECONNECT)
		MATCH_COMMANDS_SUITE_TEMPLATE  (RECONNECT)
		MATCH_MEMBERSHIP_SUITE_TEMPLATE(RECONNECT)
		MATCH_TAGS_SUITE_TEMPLATE      (RECONNECT)
	BOOST_AUTO_TEST_SUITE_END() // RECONNECT_suite

	BOOST_AUTO_TEST_SUITE(ROOMSTATE_suite)
		using Twitch::irc::message::cap::commands::ROOMSTATE;

		MATCH_RAW_SUITE_TEMPLATE       (ROOMSTATE)
		MATCH_COMMANDS_SUITE_TEMPLATE  (ROOMSTATE)
		MATCH_MEMBERSHIP_SUITE_TEMPLATE(ROOMSTATE)
		MATCH_TAGS_SUITE_TEMPLATE      (ROOMSTATE)
	BOOST_AUTO_TEST_SUITE_END() // ROOMSTATE_suite

	BOOST_AUTO_TEST_SUITE(USERNOTICE_suite)
		using Twitch::irc::message::cap::commands::USERNOTICE;

		MATCH_RAW_SUITE_TEMPLATE       (USERNOTICE)
		MATCH_COMMANDS_SUITE_TEMPLATE  (USERNOTICE)
		MATCH_MEMBERSHIP_SUITE_TEMPLATE(USERNOTICE)
		MATCH_TAGS_SUITE_TEMPLATE      (USERNOTICE)
	BOOST_AUTO_TEST_SUITE_END() // USERNOTICE_suite

	BOOST_AUTO_TEST_SUITE(USERSTATE_suite)
		using Twitch::irc::message::cap::commands::USERSTATE;

		MATCH_RAW_SUITE_TEMPLATE       (USERSTATE)
		MATCH_COMMANDS_SUITE_TEMPLATE  (USERSTATE)
		MATCH_MEMBERSHIP_SUITE_TEMPLATE(USERSTATE)
		MATCH_TAGS_SUITE_TEMPLATE      (USERSTATE)
	BOOST_AUTO_TEST_SUITE_END() // USERSTATE_suite
BOOST_AUTO_TEST_SUITE_END() // commands_tests_suite

BOOST_AUTO_TEST_SUITE(membership_tests_suite)
	BOOST_AUTO_TEST_SUITE(JOIN_suite)
		using Twitch::irc::message::cap::membership::JOIN;
		
		MATCH_RAW_SUITE_TEMPLATE       (JOIN)
		MATCH_COMMANDS_SUITE_TEMPLATE  (JOIN)
		MATCH_MEMBERSHIP_SUITE_TEMPLATE(JOIN)
		MATCH_TAGS_SUITE_TEMPLATE      (JOIN)
	BOOST_AUTO_TEST_SUITE_END() // JOIN_suite

	BOOST_AUTO_TEST_SUITE(MODE_suite)
		using Twitch::irc::message::cap::membership::MODE;
		
		MATCH_RAW_SUITE_TEMPLATE       (MODE)
		MATCH_COMMANDS_SUITE_TEMPLATE  (MODE)
		MATCH_MEMBERSHIP_SUITE_TEMPLATE(MODE)
		MATCH_TAGS_SUITE_TEMPLATE      (MODE)
	BOOST_AUTO_TEST_SUITE_END() // MODE_suite

	BOOST_AUTO_TEST_SUITE(NAMES_suite)
		using Twitch::irc::message::cap::membership::NAMES;
		
		MATCH_RAW_SUITE_TEMPLATE       (NAMES)
		MATCH_COMMANDS_SUITE_TEMPLATE  (NAMES)
		MATCH_MEMBERSHIP_SUITE_TEMPLATE(NAMES)
		MATCH_TAGS_SUITE_TEMPLATE      (NAMES)
	BOOST_AUTO_TEST_SUITE_END() // NAMES_suite

	BOOST_AUTO_TEST_SUITE(PART_suite)
		using Twitch::irc::message::cap::membership::PART;
		
		MATCH_RAW_SUITE_TEMPLATE       (PART)
		MATCH_COMMANDS_SUITE_TEMPLATE  (PART)
		MATCH_MEMBERSHIP_SUITE_TEMPLATE(PART)
		MATCH_TAGS_SUITE_TEMPLATE      (PART)
	BOOST_AUTO_TEST_SUITE_END() // PART_suite
BOOST_AUTO_TEST_SUITE_END() // membership_tests_suite

BOOST_AUTO_TEST_SUITE(tags_tests_suite)
	BOOST_AUTO_TEST_SUITE(CLEARCHAT_suite)
		using Twitch::irc::message::cap::tags::CLEARCHAT;

		MATCH_RAW_SUITE_TEMPLATE       (CLEARCHAT)
		MATCH_COMMANDS_SUITE_TEMPLATE  (CLEARCHAT)
		MATCH_MEMBERSHIP_SUITE_TEMPLATE(CLEARCHAT)
		MATCH_TAGS_SUITE_TEMPLATE      (CLEARCHAT)
	BOOST_AUTO_TEST_SUITE_END() // CLEARCHAT_suite
	
	BOOST_AUTO_TEST_SUITE(GLOBALUSERSTATE_suite)
		using Twitch::irc::message::cap::tags::GLOBALUSERSTATE;

		MATCH_RAW_SUITE_TEMPLATE       (GLOBALUSERSTATE)
		MATCH_COMMANDS_SUITE_TEMPLATE  (GLOBALUSERSTATE)
		MATCH_MEMBERSHIP_SUITE_TEMPLATE(GLOBALUSERSTATE)
		MATCH_TAGS_SUITE_TEMPLATE      (GLOBALUSERSTATE)
	BOOST_AUTO_TEST_SUITE_END() // GLOBALUSERSTATE_suite
	
	BOOST_AUTO_TEST_SUITE(PRIVMSG_suite)
		using Twitch::irc::message::cap::tags::PRIVMSG;

		MATCH_RAW_SUITE_TEMPLATE       (PRIVMSG)
		MATCH_COMMANDS_SUITE_TEMPLATE  (PRIVMSG)
		MATCH_MEMBERSHIP_SUITE_TEMPLATE(PRIVMSG)
		MATCH_TAGS_SUITE_TEMPLATE      (PRIVMSG)
	BOOST_AUTO_TEST_SUITE_END() // PRIVMSG_suite

	BOOST_AUTO_TEST_SUITE(ROOMSTATE_suite)
		using Twitch::irc::message::cap::tags::ROOMSTATE;

		MATCH_RAW_SUITE_TEMPLATE       (ROOMSTATE)
		MATCH_COMMANDS_SUITE_TEMPLATE  (ROOMSTATE)
		MATCH_MEMBERSHIP_SUITE_TEMPLATE(ROOMSTATE)
		MATCH_TAGS_SUITE_TEMPLATE      (ROOMSTATE)
	BOOST_AUTO_TEST_SUITE_END() // ROOMSTATE_suite

	BOOST_AUTO_TEST_SUITE(USERNOTICE_suite)
		using Twitch::irc::message::cap::tags::USERNOTICE;

		MATCH_RAW_SUITE_TEMPLATE       (USERNOTICE)
		MATCH_COMMANDS_SUITE_TEMPLATE  (USERNOTICE)
		MATCH_MEMBERSHIP_SUITE_TEMPLATE(USERNOTICE)
		MATCH_TAGS_SUITE_TEMPLATE      (USERNOTICE)
	BOOST_AUTO_TEST_SUITE_END() // USERNOTICE_suite

	BOOST_AUTO_TEST_SUITE(USERSTATE_suite)
		using Twitch::irc::message::cap::tags::USERSTATE;

		MATCH_RAW_SUITE_TEMPLATE       (USERSTATE)
		MATCH_COMMANDS_SUITE_TEMPLATE  (USERSTATE)
		MATCH_MEMBERSHIP_SUITE_TEMPLATE(USERSTATE)
		MATCH_TAGS_SUITE_TEMPLATE      (USERSTATE)
	BOOST_AUTO_TEST_SUITE_END() // USERSTATE_suite
BOOST_AUTO_TEST_SUITE_END() // tags_tests_suite