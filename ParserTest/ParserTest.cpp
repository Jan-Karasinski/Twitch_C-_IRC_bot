#include "stdafx.h"
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#include <boost\test\unit_test.hpp>
#include "..\Twitch_C++_IRC_bot\TwitchMessage.h"
#include <vector>
#include <functional>
#include <tuple>
#include <type_traits>

// TODO: find more test cases

/// Twitch doc is incorrect in many examples, test cases mixed with real world examples
namespace doc {
	using namespace std::string_literals;
	
	namespace ping {
		using Twitch::irc::message::PING;
		
		const std::vector<std::pair<std::string, PING>> tests{
			{
				"PING :tmi.twitch.tv"s,
				PING{ "tmi.twitch.tv"s }
			}
		};
	}

	namespace privmsg {
		using Twitch::irc::message::PRIVMSG;
		
		const std::vector<std::pair<std::string, PRIVMSG>> tests{
			{
				":ronni!ronni@ronni.tmi.twitch.tv PRIVMSG #dallas :Kappa Keepo Kappa"s,
				PRIVMSG{ "ronni"s, "tmi.twitch.tv"s, "#dallas"s, "Kappa Keepo Kappa"s }
			}
		};
	}

	namespace cap {
		namespace membership {
			namespace join {
				using Twitch::irc::message::cap::membership::JOIN;

				const std::vector<std::pair<std::string, JOIN>> tests{
					{
						":ronni!ronni@ronni.tmi.twitch.tv JOIN #dallas"s,
						JOIN{ "ronni"s, "#dallas"s }
					}
				};
			}
			namespace mode {
				using Twitch::irc::message::cap::membership::MODE;

				const std::vector<std::pair<std::string, MODE>> tests{
					{
						":jtv MODE #dallas +o ronni"s,
						MODE{ "#dallas"s, true, "ronni"s }
					},
					{
						":jtv MODE #dallas -o ronni"s,
						MODE{ "#dallas"s, false, "ronni"s }
					}
				};
			}
			namespace names {
				using Twitch::irc::message::cap::membership::NAMES;

				const std::vector<std::pair<std::string, NAMES>> tests{
					{
						":ronni.tmi.twitch.tv 353 ronni = #dallas :ronni fred wilma"s,
						NAMES{ "ronni"s, "353"s, "#dallas"s, std::vector<std::string>{ "ronni"s, "fred"s, "wilma"s } }
					},
					{
						":ronni.tmi.twitch.tv 353 ronni = #dallas :barney betty"s,
						NAMES{ "ronni"s, "353"s, "#dallas"s, std::vector<std::string>{ "barney"s, "betty"s } }
					},
					{
						":ronni.tmi.twitch.tv 366 ronni #dallas :End of /NAMES list"s,
						NAMES{ "ronni"s, "366"s, "#dallas"s, std::vector<std::string>{} }
					}
				};
			}
			namespace part {
				using Twitch::irc::message::cap::membership::PART;

				const std::vector<std::pair<std::string, PART>> tests{
					{
						":ronni!ronni@ronni.tmi.twitch.tv PART #dallas"s,
						PART{ "ronni"s, "#dallas"s }
					}
				};
			}
		}
		namespace tags {
			namespace clearchat {
				using Twitch::irc::message::cap::tags::CLEARCHAT;

				const std::vector<std::pair<std::string, CLEARCHAT>> tests{
					{
						"@ban-duration=600;room-id=99999999;"
						"target-user-id=99999999;tmi-sent-ts=1524962471755"
						" :tmi.twitch.tv CLEARCHAT #channel :nick"s,
						CLEARCHAT{
							Twitch::irc::message::cap::commands::CLEARCHAT{ "#channel"s, "nick"s },
							std::chrono::seconds{ 600 },
							std::nullopt,
							"99999999"s,
							"99999999"s,
							std::chrono::seconds{ 1524962471755 }
						}
					},
					{
						"@ban-duration=1;ban-reason=test;room-id=99999999;"
						"target-user-id=99999999;tmi-sent-ts=1525028799009"
						" :tmi.twitch.tv CLEARCHAT #channel :nick"s,
						CLEARCHAT{
							Twitch::irc::message::cap::commands::CLEARCHAT{ "#channel"s, "nick"s },
							std::chrono::seconds{ 1 },
							"test"s,
							"99999999"s,
							"99999999"s,
							std::chrono::seconds{ 1525028799009 }
						}
					}
				};
			}
			namespace globaluserstate {
				using Twitch::irc::message::cap::tags::GLOBALUSERSTATE;
				using Twitch::irc::parameters::Color;
				using Twitch::irc::parameters::NoColor;

				const std::vector<std::pair<std::string, GLOBALUSERSTATE>> tests{
					{
						"@badges=;color=#0000FF;display-name=Name;"
						"emote-sets=0,33563;user-id=99999999;user-type="
						" :tmi.twitch.tv GLOBALUSERSTATE"s,
						GLOBALUSERSTATE{
							{}, Color{ 0x00, 0x00, 0xFF }, "Name"s, "0,33563"s,
							"99999999"s, Twitch::irc::message::cap::tags::UserType::empty
						}
					}
				};
			}
			namespace privmsg {
				using Twitch::irc::message::cap::tags::PRIVMSG;
				using Twitch::irc::parameters::Color;
				using Twitch::irc::parameters::NoColor;
				using Twitch::irc::parameters::Badge;
				using Twitch::irc::parameters::UserType;
				
				const std::vector<std::pair<std::string, PRIVMSG>> tests{
					{
						"@badges=staff/1,bits/1000;bits=100;color=;display-name=dallas;"
						"emotes=;id=b34ccfc7-4977-403a-8a94-33c6bac34fb8;"
						"mod=0;room-id=1337;subscriber=0;tmi-sent-ts=1507246572675;"
						"turbo=1;user-id=1337;user-type=staff"
						" :ronni!ronni@ronni.tmi.twitch.tv PRIVMSG #dallas :cheer100"s,
						PRIVMSG{
							Twitch::irc::message::PRIVMSG{ "ronni"s, "tmi.twitch.tv"s, "#dallas"s, "cheer100"s },
							{ { Badge::staff, 1 }, { Badge::bits, 1000 } }, 100, NoColor{}, "dallas"s,
							false, ""s, "b34ccfc7-4977-403a-8a94-33c6bac34fb8"s,
							false, "1337"s, false, std::chrono::seconds{ 1507246572675 },
							true, "1337"s, UserType::staff
						}
					},
					{
						"@badges=broadcaster/1;color=#0000FF;display-name=Nick;"
						"emote-only=1;emotes=25:0-4,12-16/1902:6-10;id=99999999-9999-9999-9999-999999999999;"
						"mod=0;room-id=99999999;subscriber=0;tmi-sent-ts=1526424153891;"
						"turbo=0;user-id=99999999;user-type="
						" :user!user@user.tmi.twitch.tv PRIVMSG #channel :Kappa Keepo Kappa"s,
						PRIVMSG{
							Twitch::irc::message::PRIVMSG{ "user"s, "tmi.twitch.tv"s, "#channel"s, "Kappa Keepo Kappa"s },
							{ { Badge::broadcaster, 1 } }, 0, Color{ 0x00, 0x00, 0xFF }, "Nick"s,
							true, "25:0-4,12-16/1902:6-10"s, "99999999-9999-9999-9999-999999999999"s,
							false, "99999999"s, false, std::chrono::seconds{ 1526424153891 },
							false, "99999999"s, UserType::empty
						}
					}
				};
			}
			namespace roomstate {
				using Twitch::irc::message::cap::tags::ROOMSTATE;

				const std::vector<std::pair<std::string, ROOMSTATE>> tests{
					{
						"@broadcaster-lang=;emote-only=0;followers-only=-1;"
						"r9k=0;rituals=0;room-id=99999999;slow=0;subs-only=0"
						" :tmi.twitch.tv ROOMSTATE #channel"s,
						ROOMSTATE{
							Twitch::irc::message::cap::commands::ROOMSTATE{ "#channel"s },
							std::nullopt, false, -1, false, "0"s, "99999999"s, std::chrono::seconds{ 0 }, false
						}
					},
					{
						"@room-id=99999999;slow=10 :tmi.twitch.tv ROOMSTATE #channel",
						ROOMSTATE{
							Twitch::irc::message::cap::commands::ROOMSTATE{ "#channel"s },
							std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, "99999999"s,
							std::chrono::seconds{ 10 }, std::nullopt
						}
					},
					{
						"@room-id=99999999;slow=0 :tmi.twitch.tv ROOMSTATE #channel"s,
						ROOMSTATE{
							Twitch::irc::message::cap::commands::ROOMSTATE{ "#channel"s },
							std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt,
							"99999999"s, std::chrono::seconds{ 0 }, std::nullopt
						}
					},
					{
						"@followers-only=30;room-id=99999999"
						" :tmi.twitch.tv ROOMSTATE #channel"s,
						ROOMSTATE{
							Twitch::irc::message::cap::commands::ROOMSTATE{ "#channel"s },
							std::nullopt, std::nullopt, 30, std::nullopt, std::nullopt,
							"99999999"s, std::nullopt, std::nullopt
						}
					},
					{
						"@followers-only=-1;room-id=99999999"
						" :tmi.twitch.tv ROOMSTATE #channel"s,
						ROOMSTATE{
							Twitch::irc::message::cap::commands::ROOMSTATE{ "#channel"s },
							std::nullopt, std::nullopt, -1, std::nullopt, std::nullopt,
							"99999999"s, std::nullopt, std::nullopt
						}
					}
				};
			}
			namespace usernotice {
				using Twitch::irc::message::cap::tags::USERNOTICE;
				using Twitch::irc::parameters::Color;
				using Twitch::irc::parameters::NoColor;
				using Twitch::irc::parameters::Badge;
				using Twitch::irc::parameters::UserType;

				const std::vector<std::pair<std::string, USERNOTICE>> tests{
					{
						"@badges=staff/1,broadcaster/1,turbo/1;color=#008000;display-name=ronni;"
						"emotes=;id=db25007f-7a18-43eb-9379-80131e44d633;login=ronni;"
						"mod=0;msg-id=resub;msg-param-months=6;msg-param-sub-plan=Prime;"
						"msg-param-sub-plan-name=Prime;room-id=1337;subscriber=1;"
						R"(system-msg=ronni\shas\ssubscribed\sfor\s6\smonths!;)"
						"tmi-sent-ts=1507246572675;turbo=1;user-id=1337;user-type=staff"
						" :tmi.twitch.tv USERNOTICE #dallas :Great stream -- keep it up!"s,
						USERNOTICE{
							Twitch::irc::message::cap::commands::USERNOTICE{
								"#dallas"s, "Great stream -- keep it up!"s
							},
							{ {Badge::staff, 1}, {Badge::broadcaster, 1}, {Badge::turbo, 1} },
							Color{ 0x00, 0x80, 0x00 }, "ronni"s, ""s, "db25007f-7a18-43eb-9379-80131e44d633"s,
							"ronni"s, false, USERNOTICE::Sub{ 6, "Prime"s, "Prime"s }, "1337"s,
							true, "ronni has subscribed for 6 months!"s,
							std::chrono::seconds{ 1507246572675 }, true, "1337"s, UserType::staff
						}
					},
					{
						"@badges=staff/1,premium/1;color=#0000FF;display-name=TWW2;"
						"emotes=;id=e9176cd8-5e22-4684-ad40-ce53c2561c5e;login=tww2;"
						"mod=0;msg-id=subgift;msg-param-months=1;"
						"msg-param-recipient-display-name=Mr_Woodchuck;"
						"msg-param-recipient-id=89614178;msg-param-recipient-name=mr_woodchuck;"
						R"(msg-param-sub-plan-name=House\sof\sNyoro~n;msg-param-sub-plan=1000;)"
						"room-id=19571752;subscriber=0;"
						R"(system-msg=TWW2\sgifted\sa\sTier\s1\ssub\sto\sMr_Woodchuck!;)"
						"tmi-sent-ts=1521159445153;turbo=0;user-id=13405587;"
						"user-type=staff :tmi.twitch.tv USERNOTICE #forstycup"s,
						USERNOTICE{
							Twitch::irc::message::cap::commands::USERNOTICE{
								"#forstycup"s, ""s
							},
							// premium badge - wtf??? doc don't say a word about it
							{ {Badge::staff, 1}, {Badge::unhandled_badge, 1} },
							Color{ 00, 00, 0xFF }, "TWW2"s, ""s, "e9176cd8-5e22-4684-ad40-ce53c2561c5e"s,
							"tww2"s, false,
							USERNOTICE::Subgift{
								1, "Mr_Woodchuck"s, "89614178"s, "mr_woodchuck"s,
								"House of Nyoro~n"s, "1000"s
							}, "19571752"s, false, "TWW2 gifted a Tier 1 sub to Mr_Woodchuck!"s,
							std::chrono::seconds{ 1521159445153 }, false, "13405587"s, UserType::staff
						}
					},
					{
						"@badges=turbo/1;color=#9ACD32;display-name=TestChannel;emotes=;"
						"id=3d830f12-795c-447d-af3c-ea05e40fbddb;login=testchannel;mod=0;"
						"msg-id=raid;msg-param-displayName=TestChannel;msg-param-login=testchannel;"
						"msg-param-viewerCount=15;room-id=56379257;subscriber=0;"
						R"(system-msg=15\sraiders\sfrom\sTestChannel\shave\sjoined\n!;)"
						"tmi-sent-ts=1507246572675;turbo=1;user-id=123456;user-type="
						" :tmi.twitch.tv USERNOTICE #othertestchannel"s,
						USERNOTICE{
							Twitch::irc::message::cap::commands::USERNOTICE{
								"#othertestchannel"s, ""s
							},
							{ {Badge::turbo, 1} },
							Color{ 0x9A, 0xCD, 0x32 }, "TestChannel"s, ""s, "3d830f12-795c-447d-af3c-ea05e40fbddb"s,
							"testchannel"s, false,
							USERNOTICE::Raid{
								"TestChannel"s, "testchannel"s,  15
							}, "56379257"s, false, R"(15 raiders from TestChannel have joined\n!)"s,
							std::chrono::seconds{ 1507246572675 }, true, "123456"s, UserType::empty
						}
					},
					{
						"@badges=;color=;display-name=SevenTest1;emotes=30259:0-6;"
						"id=37feed0f-b9c7-4c3a-b475-21c6c6d21c3d;login=seventest1;"
						"mod=0;msg-id=ritual;msg-param-ritual-name=new_chatter;"
						R"(room-id=6316121;subscriber=0;system-msg=Seventoes\sis\snew\shere!;)"
						"tmi-sent-ts=1508363903826;turbo=0;user-id=131260580;"
						"user-type= :tmi.twitch.tv USERNOTICE #seventoes :HeyGuys"s,
						USERNOTICE{
							Twitch::irc::message::cap::commands::USERNOTICE{
								"#seventoes"s, "HeyGuys"s
							},
							{}, NoColor{},
							"SevenTest1"s, "30259:0-6"s, "37feed0f-b9c7-4c3a-b475-21c6c6d21c3d"s,
							"seventest1"s, false,
							USERNOTICE::Ritual{},
							"6316121"s, false, "Seventoes is new here!"s,
							std::chrono::seconds{ 1508363903826 }, false, "131260580"s, UserType::empty
						}
					}
				};
			}
			namespace userstate {
				using Twitch::irc::message::cap::tags::USERSTATE;
				using Twitch::irc::parameters::Color;
				using Twitch::irc::parameters::Badge;
				using Twitch::irc::parameters::UserType;

				const std::vector<std::pair<std::string, USERSTATE>> tests{
					{
						"@badges=broadcaster/1;color=#0000FF;"
						"display-name=Nick;emote-sets=0,33563;"
						"mod=0;subscriber=0;user-type="
						" :tmi.twitch.tv USERSTATE #channel"s,
						USERSTATE{
							Twitch::irc::message::cap::commands::USERSTATE{ "#channel"s },
							{ {Badge::broadcaster, 1} }, Color{ 00, 00, 0xFF}, "Nick"s, "0,33563"s,
							false, false, UserType::empty
						}
					}
				};
			}
		}
		namespace commands {
			namespace clearchat {
				using Twitch::irc::message::cap::commands::CLEARCHAT;

				const std::vector<std::pair<std::string, CLEARCHAT>> tests{
					{
						":tmi.twitch.tv CLEARCHAT #dallas"s,
						CLEARCHAT{ "#dallas"s, ""s }
					},
					{
						":tmi.twitch.tv CLEARCHAT #<channel> :<user>"s,
						CLEARCHAT{ "#<channel>"s, "<user>"s }
					}
				};
			}
			namespace hosttarget {
				using Twitch::irc::message::cap::commands::HOSTTARGET;

				// TODO: find more rwe
				const std::vector<std::pair<std::string, HOSTTARGET>> tests{
					{
						":tmi.twitch.tv HOSTTARGET #hosting_channel <channel> [0]"s,
						HOSTTARGET{ "#hosting_channel"s, "<channel>"s, 0 }
					},
					{
						":tmi.twitch.tv HOSTTARGET #hosting :channel -"s,
						HOSTTARGET{ "#hosting"s, "channel"s, 0 }
					},
					{
						":tmi.twitch.tv HOSTTARGET #hosting_channel :- [0]"s,
						HOSTTARGET{ "#hosting_channel"s, ""s, 0 }
					}
				};
			}
			namespace notice {
				using Twitch::irc::message::cap::commands::NOTICE;

				const std::vector<std::pair<std::string, NOTICE>> tests{
					{
						"@msg-id=slow_off :tmi.twitch.tv NOTICE"
						" #dallas :This room is no longer in slow mode."s,
						NOTICE{ "slow_off"s, "#dallas"s, "This room is no longer in slow mode."s }
					}
				};
			}
			namespace reconnect {
				using Twitch::irc::message::cap::commands::RECONNECT;

				const std::vector<std::pair<std::string, RECONNECT>> tests{
					{
						":tmi.twitch.tv RECONNECT"s,
						RECONNECT{}
					}
				};
			}
			namespace roomstate {
				using Twitch::irc::message::cap::commands::ROOMSTATE;

				const std::vector<std::pair<std::string, ROOMSTATE>> tests{
					{
						":tmi.twitch.tv ROOMSTATE #<channel>"s,
						ROOMSTATE{ "#<channel>"s }
					}
				};
			}
			namespace usernotice {
				using Twitch::irc::message::cap::commands::USERNOTICE;

				const std::vector<std::pair<std::string, USERNOTICE>> tests{
					{
						":tmi.twitch.tv USERNOTICE #<channel> :message"s,
						USERNOTICE{ "#<channel>"s, "message"s }
					}
				};
			}
			namespace userstate {
				using Twitch::irc::message::cap::commands::USERSTATE;

				const std::vector<std::pair<std::string, USERSTATE>> tests{
					{
						":tmi.twitch.tv USERSTATE #<channel>"s,
						USERSTATE{ "#<channel>"s }
					}
				};
			}
		}
	}
}

namespace {
	namespace message = Twitch::irc::message;
	namespace message_doc = doc;

	namespace tags = Twitch::irc::message::cap::tags;
	namespace tags_doc = doc::cap::tags;

	namespace commands = Twitch::irc::message::cap::commands;
	namespace commands_doc = doc::cap::commands;

	namespace membership  = Twitch::irc::message::cap::membership;
	namespace membership_doc = doc::cap::membership;

	template<class M> std::string get_message_t_name() = delete;

	template<> std::string get_message_t_name<message::PING>()    { return "PING"         ; };
	template<> std::string get_message_t_name<message::PRIVMSG>() { return "basic_PRIVMSG"; };
	
	template<> std::string get_message_t_name<commands::CLEARCHAT>()  { return "commands__CLEARCHAT" ; };
	template<> std::string get_message_t_name<commands::HOSTTARGET>() { return "commands__HOSTTARGET"; };
	template<> std::string get_message_t_name<commands::NOTICE>()     { return "commands__NOTICE"    ; };
	template<> std::string get_message_t_name<commands::RECONNECT>()  { return "commands__RECONNECT" ; };
	template<> std::string get_message_t_name<commands::ROOMSTATE>()  { return "commands__ROOMSTATE" ; };
	template<> std::string get_message_t_name<commands::USERNOTICE>() { return "commands__USERNOTICE"; };
	template<> std::string get_message_t_name<commands::USERSTATE>()  { return "commands__USERSTATE" ; };
	
	template<> std::string get_message_t_name<membership::JOIN>()  { return "membership__JOIN" ; };
	template<> std::string get_message_t_name<membership::MODE>()  { return "membership__MODE" ; };
	template<> std::string get_message_t_name<membership::NAMES>() { return "membership__NAMES"; };
	template<> std::string get_message_t_name<membership::PART>()  { return "membership__PART" ; };

	template<> std::string get_message_t_name<tags::CLEARCHAT>()       { return "tags__CLEARCHAT"      ; };
	template<> std::string get_message_t_name<tags::GLOBALUSERSTATE>() { return "tags__GLOBALUSERSTATE"; };
	template<> std::string get_message_t_name<tags::PRIVMSG>()         { return "tags__PRIVMSG"        ; };
	template<> std::string get_message_t_name<tags::ROOMSTATE>()       { return "tags__ROOMSTATE"      ; };
	template<> std::string get_message_t_name<tags::USERNOTICE>()      { return "tags__USERNOTICE"     ; };
	template<> std::string get_message_t_name<tags::USERSTATE>()       { return "tags__USERSTATE"      ; };

	template<class Tested_Message_t, class Message_t>
	void match_impl(const std::vector<std::pair<std::string, Message_t>>& tests) {
		for (const auto& [message, parsed] : tests) {
			const auto tp = Tested_Message_t::is(message);
			if constexpr (std::is_same_v<Tested_Message_t, Message_t>) {
				BOOST_CHECK(tp.has_value() && tp.value() == parsed);
			}
			else {
				BOOST_CHECK(!tp.has_value());
			}
		}
	}

	template<class M>
	struct match_basic_messages_details {
		static void match_with_PING();
		static void match_with_PRIVMSG();
	};

	template<class M> void match_basic_messages_details<M>::match_with_PING() {
		match_impl<M, message::PING>(message_doc::ping::tests);
	}
	template<class M> void match_basic_messages_details<M>::match_with_PRIVMSG() {
		match_impl<M, message::PRIVMSG>(message_doc::privmsg::tests);
	}

	template<class M>
	struct match_commands_details {
		static void match_with_CLEARCHAT();
		static void match_with_HOSTTARGET();
		static void match_with_NOTICE();
		static void match_with_RECONNECT();
		static void match_with_ROOMSTATE();
		static void match_with_USERNOTICE();
		static void match_with_USERSTATE();
	};

	template<class M> void match_commands_details<M>::match_with_CLEARCHAT() {
		match_impl<M, commands::CLEARCHAT>(commands_doc::clearchat::tests);
	}
	template<class M> void match_commands_details<M>::match_with_HOSTTARGET() {
		match_impl<M, commands::HOSTTARGET>(commands_doc::hosttarget::tests);
	}
	template<class M> void match_commands_details<M>::match_with_NOTICE() {
		match_impl<M, commands::NOTICE>(commands_doc::notice::tests);
	}
	template<class M> void match_commands_details<M>::match_with_RECONNECT() {
		match_impl<M, commands::RECONNECT>(commands_doc::reconnect::tests);
	}
	template<class M> void match_commands_details<M>::match_with_ROOMSTATE() {
		match_impl<M, commands::ROOMSTATE>(commands_doc::roomstate::tests);
	}
	template<class M> void match_commands_details<M>::match_with_USERNOTICE() {
		match_impl<M, commands::USERNOTICE>(commands_doc::usernotice::tests);
	}
	template<class M> void match_commands_details<M>::match_with_USERSTATE() {
		match_impl<M, commands::USERSTATE>(commands_doc::userstate::tests);
	}

	template<class M>
	struct match_membership_details {
		static void match_with_JOIN();
		static void match_with_MODE();
		static void match_with_NAMES();
		static void match_with_PART();
	};

	template<class M> void match_membership_details<M>::match_with_JOIN() {
		match_impl<M, membership::JOIN>(membership_doc::join::tests);
	}
	template<class M> void match_membership_details<M>::match_with_MODE() {
		match_impl<M, membership::MODE>(membership_doc::mode::tests);
	}
	template<class M> void match_membership_details<M>::match_with_NAMES() {
		match_impl<M, membership::NAMES>(membership_doc::names::tests);
	}
	template<class M> void match_membership_details<M>::match_with_PART() {
		match_impl<M, membership::PART>(membership_doc::part::tests);
	}

	// fns used to test type against tags cap messages
	template<class M>
	struct match_tags_details {
		static void match_with_CLEARCHAT();
		static void match_with_GLOBALUSERSTATE();
		static void match_with_PRIVMSG();
		static void match_with_ROOMSTATE();
		static void match_with_USERNOTICE();
		static void match_with_USERSTATE();
	};

	template<class M> void match_tags_details<M>::match_with_CLEARCHAT() {
		match_impl<M, tags::CLEARCHAT>(tags_doc::clearchat::tests);
	}
	template<class M> void match_tags_details<M>::match_with_GLOBALUSERSTATE() {
		match_impl<M, tags::GLOBALUSERSTATE>(tags_doc::globaluserstate::tests);
	}
	template<class M> void match_tags_details<M>::match_with_PRIVMSG() {
		match_impl<M, tags::PRIVMSG>(tags_doc::privmsg::tests);
	}
	template<class M> void match_tags_details<M>::match_with_ROOMSTATE() {
		match_impl<M, tags::ROOMSTATE>(tags_doc::roomstate::tests);
	}
	template<class M> void match_tags_details<M>::match_with_USERNOTICE() {
		match_impl<M, tags::USERNOTICE>(tags_doc::usernotice::tests);
	}
	template<class M> void match_tags_details<M>::match_with_USERSTATE() {
		match_impl<M, tags::USERSTATE>(tags_doc::userstate::tests);
	}
	
	template<class M> auto* match_basic_messages_suite(const std::string& suite_name) {
		auto* suite = BOOST_TEST_SUITE(std::move(suite_name));

		auto* c1 = BOOST_TEST_CASE( &match_basic_messages_details<M>::match_with_PING    );
		auto* c2 = BOOST_TEST_CASE( &match_basic_messages_details<M>::match_with_PRIVMSG );
	
		{
			using namespace std::string_literals;
			c1->p_name.set("match_"s + get_message_t_name<M>() + "_with_message__PING"s );
			c2->p_name.set("match_"s + get_message_t_name<M>() + "_with_message__PRIVMSG"s);
		}

		suite->add(c1);
		suite->add(c2);

		return suite;
	}

	template<class M> auto* match_commands_suite(const std::string& suite_name) {
		auto* suite = BOOST_TEST_SUITE(std::move(suite_name));

		auto* c1 = BOOST_TEST_CASE( &match_commands_details<M>::match_with_CLEARCHAT  );
		auto* c2 = BOOST_TEST_CASE( &match_commands_details<M>::match_with_HOSTTARGET );
		auto* c3 = BOOST_TEST_CASE( &match_commands_details<M>::match_with_NOTICE     );
		auto* c4 = BOOST_TEST_CASE( &match_commands_details<M>::match_with_RECONNECT  );
		auto* c5 = BOOST_TEST_CASE( &match_commands_details<M>::match_with_ROOMSTATE  );
		auto* c6 = BOOST_TEST_CASE( &match_commands_details<M>::match_with_USERNOTICE );
		auto* c7 = BOOST_TEST_CASE( &match_commands_details<M>::match_with_USERSTATE  );
	
		{
			using namespace std::string_literals;
			c1->p_name.set("match_"s + get_message_t_name<M>() + "_with_commands__CLEARCHAT"s );
			c2->p_name.set("match_"s + get_message_t_name<M>() + "_with_commands__HOSTTARGET"s);
			c3->p_name.set("match_"s + get_message_t_name<M>() + "_with_commands__NOTICE"s    );
			c4->p_name.set("match_"s + get_message_t_name<M>() + "_with_commands__RECONNECT"s );
			c5->p_name.set("match_"s + get_message_t_name<M>() + "_with_commands__ROOMSTATE"s );
			c6->p_name.set("match_"s + get_message_t_name<M>() + "_with_commands__USERNOTICE"s);
			c7->p_name.set("match_"s + get_message_t_name<M>() + "_with_commands__USERSTATE"s );
		}

		suite->add(c1);
		suite->add(c2);
		suite->add(c3);
		suite->add(c4);
		suite->add(c5);
		suite->add(c6);
		suite->add(c7);

		return suite;
	}

	template<class M> auto* match_membership_suite(const std::string& suite_name) {
		auto* suite = BOOST_TEST_SUITE(std::move(suite_name));

		auto* c1 = BOOST_TEST_CASE( &match_membership_details<M>::match_with_JOIN  );
		auto* c2 = BOOST_TEST_CASE( &match_membership_details<M>::match_with_MODE  );
		auto* c3 = BOOST_TEST_CASE( &match_membership_details<M>::match_with_NAMES );
		auto* c4 = BOOST_TEST_CASE( &match_membership_details<M>::match_with_PART  );

		{
			using namespace std::string_literals;
			c1->p_name.set("match_"s + get_message_t_name<M>() + "_with_membership__JOIN"s);
			c2->p_name.set("match_"s + get_message_t_name<M>() + "_with_membership__MODE"s);
			c3->p_name.set("match_"s + get_message_t_name<M>() + "_with_membership__NAMES"s);
			c4->p_name.set("match_"s + get_message_t_name<M>() + "_with_membership__PART"s);
		}

		suite->add(c1);
		suite->add(c2);
		suite->add(c3);
		suite->add(c4);

		return suite;
	}

	template<class M> auto* match_tags_suite(const std::string& suite_name) {
		namespace tags = Twitch::irc::message::cap::tags;
		namespace doc = doc::cap::tags;
	
		auto* suite = BOOST_TEST_SUITE(std::move(suite_name));

		auto* c1 = BOOST_TEST_CASE( &match_tags_details<M>::match_with_CLEARCHAT       );
		auto* c2 = BOOST_TEST_CASE( &match_tags_details<M>::match_with_GLOBALUSERSTATE );
		auto* c3 = BOOST_TEST_CASE( &match_tags_details<M>::match_with_PRIVMSG         );
		auto* c4 = BOOST_TEST_CASE( &match_tags_details<M>::match_with_ROOMSTATE       );
		auto* c5 = BOOST_TEST_CASE( &match_tags_details<M>::match_with_USERNOTICE      );
		auto* c6 = BOOST_TEST_CASE( &match_tags_details<M>::match_with_USERSTATE       );

		{
			using namespace std::string_literals;
			c1->p_name.set("match_"s + get_message_t_name<M>() + "_with_tags__CLEARCHAT"s      );
			c2->p_name.set("match_"s + get_message_t_name<M>() + "_with_tags__GLOBALUSERSTATE"s);
			c3->p_name.set("match_"s + get_message_t_name<M>() + "_with_tags__PRIVMSG"s        );
			c4->p_name.set("match_"s + get_message_t_name<M>() + "_with_tags__ROOMSTATE"s      );
			c5->p_name.set("match_"s + get_message_t_name<M>() + "_with_tags__USERNOTICE"s     );
			c6->p_name.set("match_"s + get_message_t_name<M>() + "_with_tags__USERSTATE"s      );
		}

		suite->add(c1);
		suite->add(c2);
		suite->add(c3);
		suite->add(c4);
		suite->add(c5);
		suite->add(c6);

		return suite;
	}
}
// TODO: clean up
boost::unit_test::test_suite* init_unit_test_suite(int /*argc*/, char* /*argv*/[])
{
	using namespace std::string_literals;
	boost::unit_test::framework::master_test_suite().p_name.set("ParserTest");
	{
		auto* basic_messages_suite = BOOST_TEST_SUITE("basic_messages_suite");
		{
			namespace message = Twitch::irc::message;
			basic_messages_suite->add( match_basic_messages_suite<message::PING>   ("PING_suite"s)          );
			basic_messages_suite->add( match_basic_messages_suite<message::PRIVMSG>("basic_PRIVMSG_suite"s) );
			
			basic_messages_suite->add( match_commands_suite<message::PING>   ("PING_suite"s)          );
			basic_messages_suite->add( match_commands_suite<message::PRIVMSG>("basic_PRIVMSG_suite"s) );

			basic_messages_suite->add( match_membership_suite<message::PING>   ("PING_suite"s)          );
			basic_messages_suite->add( match_membership_suite<message::PRIVMSG>("basic_PRIVMSG_suite"s) );
			
			basic_messages_suite->add( match_tags_suite<message::PING>   ("PING_suite"s)          );
			basic_messages_suite->add( match_tags_suite<message::PRIVMSG>("basic_PRIVMSG_suite"s) );
		}

		boost::unit_test::framework::master_test_suite().add(basic_messages_suite);
	}
	{
		auto* commands_suite = BOOST_TEST_SUITE("commands_suite");
		{
			namespace commands = Twitch::irc::message::cap::commands;
			commands_suite->add( match_basic_messages_suite<commands::CLEARCHAT> ("CLEARCHAT_suite"s)  );
			commands_suite->add( match_basic_messages_suite<commands::HOSTTARGET>("HOSTTARGET_suite"s) );
			commands_suite->add( match_basic_messages_suite<commands::NOTICE>    ("NOTICE_suite"s)     );
			commands_suite->add( match_basic_messages_suite<commands::RECONNECT> ("RECONNECT_suite"s)  );
			commands_suite->add( match_basic_messages_suite<commands::ROOMSTATE> ("ROOMSTATE_suite"s)  );
			commands_suite->add( match_basic_messages_suite<commands::USERNOTICE>("USERNOTICE_suite"s) );
			commands_suite->add( match_basic_messages_suite<commands::USERSTATE> ("USERSTATE_suite"s)  );

			commands_suite->add( match_commands_suite<commands::CLEARCHAT> ("CLEARCHAT_suite"s)  );
			commands_suite->add( match_commands_suite<commands::HOSTTARGET>("HOSTTARGET_suite"s) );
			commands_suite->add( match_commands_suite<commands::NOTICE>    ("NOTICE_suite"s)     );
			commands_suite->add( match_commands_suite<commands::RECONNECT> ("RECONNECT_suite"s)  );
			commands_suite->add( match_commands_suite<commands::ROOMSTATE> ("ROOMSTATE_suite"s)  );
			commands_suite->add( match_commands_suite<commands::USERNOTICE>("USERNOTICE_suite"s) );
			commands_suite->add( match_commands_suite<commands::USERSTATE> ("USERSTATE_suite"s)  );
			
			commands_suite->add( match_membership_suite<commands::CLEARCHAT> ("CLEARCHAT_suite"s)  );
			commands_suite->add( match_membership_suite<commands::HOSTTARGET>("HOSTTARGET_suite"s) );
			commands_suite->add( match_membership_suite<commands::NOTICE>    ("NOTICE_suite"s)     );
			commands_suite->add( match_membership_suite<commands::RECONNECT> ("RECONNECT_suite"s)  );
			commands_suite->add( match_membership_suite<commands::ROOMSTATE> ("ROOMSTATE_suite"s)  );
			commands_suite->add( match_membership_suite<commands::USERNOTICE>("USERNOTICE_suite"s) );
			commands_suite->add( match_membership_suite<commands::USERSTATE> ("USERSTATE_suite"s)  );
			
			commands_suite->add( match_tags_suite<commands::CLEARCHAT> ("CLEARCHAT_suite"s)  );
			commands_suite->add( match_tags_suite<commands::HOSTTARGET>("HOSTTARGET_suite"s) );
			commands_suite->add( match_tags_suite<commands::NOTICE>    ("NOTICE_suite"s)     );
			commands_suite->add( match_tags_suite<commands::RECONNECT> ("RECONNECT_suite"s)  );
			commands_suite->add( match_tags_suite<commands::ROOMSTATE> ("ROOMSTATE_suite"s)  );
			commands_suite->add( match_tags_suite<commands::USERNOTICE>("USERNOTICE_suite"s) );
			commands_suite->add( match_tags_suite<commands::USERSTATE> ("USERSTATE_suite"s)  );
		}

		boost::unit_test::framework::master_test_suite().add(commands_suite);
	}
	{
		auto* membership_suite = BOOST_TEST_SUITE("membership_suite");
		{
			namespace membership = Twitch::irc::message::cap::membership;
			membership_suite->add( match_basic_messages_suite<membership::JOIN> ("JOIN_suite"s)  );
			membership_suite->add( match_basic_messages_suite<membership::MODE> ("MODE_suite"s)  );
			membership_suite->add( match_basic_messages_suite<membership::NAMES>("NAMES_suite"s) );
			membership_suite->add( match_basic_messages_suite<membership::PART> ("PART_suite"s)  );

			membership_suite->add( match_commands_suite<membership::JOIN> ("JOIN_suite"s)  );
			membership_suite->add( match_commands_suite<membership::MODE> ("MODE_suite"s)  );
			membership_suite->add( match_commands_suite<membership::NAMES>("NAMES_suite"s) );
			membership_suite->add( match_commands_suite<membership::PART> ("PART_suite"s)  );
			
			membership_suite->add( match_membership_suite<membership::JOIN> ("JOIN_suite"s)  );
			membership_suite->add( match_membership_suite<membership::MODE> ("MODE_suite"s)  );
			membership_suite->add( match_membership_suite<membership::NAMES>("NAMES_suite"s) );
			membership_suite->add( match_membership_suite<membership::PART> ("PART_suite"s)  );
			
			membership_suite->add( match_tags_suite<membership::JOIN> ("JOIN_suite"s)  );
			membership_suite->add( match_tags_suite<membership::MODE> ("MODE_suite"s)  );
			membership_suite->add( match_tags_suite<membership::NAMES>("NAMES_suite"s) );
			membership_suite->add( match_tags_suite<membership::PART> ("PART_suite"s)  );
		}

		boost::unit_test::framework::master_test_suite().add(membership_suite);
	}
	{
		auto* tags_suite = BOOST_TEST_SUITE("tags_suite");
		{
			namespace tags = Twitch::irc::message::cap::tags;
			tags_suite->add( match_basic_messages_suite<tags::CLEARCHAT>      ("CLEARCHAT_suite"s)       );
			tags_suite->add( match_basic_messages_suite<tags::GLOBALUSERSTATE>("GLOBALUSERSTATE_suite"s) );
			tags_suite->add( match_basic_messages_suite<tags::PRIVMSG>        ("PRIVMSG_suite"s)         );
			tags_suite->add( match_basic_messages_suite<tags::ROOMSTATE>      ("ROOMSTATE_suite"s)       );
			tags_suite->add( match_basic_messages_suite<tags::USERNOTICE>     ("USERNOTICE_suite"s)      );
			tags_suite->add( match_basic_messages_suite<tags::USERSTATE>      ("USERSTATE_suite"s)       );
			
			tags_suite->add( match_commands_suite<tags::CLEARCHAT>      ("CLEARCHAT_suite"s)       );
			tags_suite->add( match_commands_suite<tags::GLOBALUSERSTATE>("GLOBALUSERSTATE_suite"s) );
			tags_suite->add( match_commands_suite<tags::PRIVMSG>        ("PRIVMSG_suite"s)         );
			tags_suite->add( match_commands_suite<tags::ROOMSTATE>      ("ROOMSTATE_suite"s)       );
			tags_suite->add( match_commands_suite<tags::USERNOTICE>     ("USERNOTICE_suite"s)      );
			tags_suite->add( match_commands_suite<tags::USERSTATE>      ("USERSTATE_suite"s)       );
			
			tags_suite->add( match_membership_suite<tags::CLEARCHAT>      ("CLEARCHAT_suite"s)       );
			tags_suite->add( match_membership_suite<tags::GLOBALUSERSTATE>("GLOBALUSERSTATE_suite"s) );
			tags_suite->add( match_membership_suite<tags::PRIVMSG>        ("PRIVMSG_suite"s)         );
			tags_suite->add( match_membership_suite<tags::ROOMSTATE>      ("ROOMSTATE_suite"s)       );
			tags_suite->add( match_membership_suite<tags::USERNOTICE>     ("USERNOTICE_suite"s)      );
			tags_suite->add( match_membership_suite<tags::USERSTATE>      ("USERSTATE_suite"s)       );
			
			tags_suite->add( match_tags_suite<tags::CLEARCHAT>      ("CLEARCHAT_suite"s)       );
			tags_suite->add( match_tags_suite<tags::GLOBALUSERSTATE>("GLOBALUSERSTATE_suite"s) );
			tags_suite->add( match_tags_suite<tags::PRIVMSG>        ("PRIVMSG_suite"s)         );
			tags_suite->add( match_tags_suite<tags::ROOMSTATE>      ("ROOMSTATE_suite"s)       );
			tags_suite->add( match_tags_suite<tags::USERNOTICE>     ("USERNOTICE_suite"s)      );
			tags_suite->add( match_tags_suite<tags::USERSTATE>      ("USERSTATE_suite"s)       );
		}

		boost::unit_test::framework::master_test_suite().add(tags_suite);
	}

	return 0;
}
