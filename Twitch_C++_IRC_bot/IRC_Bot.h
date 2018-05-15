#ifndef IRC_BOT_H
#define IRC_BOT_H
#include "TwitchMessage.h"
#include <WinSock2.h>
#include <boost\asio.hpp>
#include <chrono>
#include <memory>
#include <map>
#include <string>
#include <vector>
#include <mutex>

namespace Twitch::irc {
	namespace message {
		class MessageParser;
		namespace cap::tags {
			struct PRIVMSG;
		}
	}
	using io_service_t = boost::asio::io_service;
	using resolver_t   = boost::asio::ip::tcp::resolver;
	using socket_t     = boost::asio::ip::tcp::socket;
	using streambuf_t  = boost::asio::streambuf;
	using error_code_t = boost::system::error_code;
	
	struct Commands
	{ // TODO: thread safety
		using key_type = std::string;
		using cmd_handle_t = std::function<std::string(message::cap::tags::PRIVMSG const&)>;
		using value_type = std::map<std::string, cmd_handle_t>::value_type;
		
		static const std::string cmd_indicator;
		
		cmd_handle_t find(const std::string& key) const;

		Commands(std::initializer_list<value_type> init);
		Commands(const Commands& c);
		Commands& operator=(const Commands& c);

	private:
		//Commands() = default;

		std::map<std::string, cmd_handle_t> m_commands;
		mutable std::mutex m_mutex;
	};

	struct IRCReader
	{
		virtual std::pair<error_code_t, std::string> read() = 0;
		virtual ~IRCReader() = default;
	};

	struct IRCWriter
	{
		virtual error_code_t write(const std::string&) = 0;
		virtual ~IRCWriter() = default;
	};

	struct IController : public IRCReader, public IRCWriter
	{
		virtual error_code_t connect() = 0;
		virtual error_code_t login() = 0;
		virtual error_code_t join_channel() = 0;
		virtual error_code_t cap_req(const std::string& cap) = 0;
		virtual error_code_t reconnect() = 0;
		virtual bool is_alive() const noexcept = 0;
		~IController() override = default;
	};

	class Controller : public IController
	{ /// https://dev.twitch.tv/docs/irc#connecting-to-twitch-irc
	public:
		Controller(
			std::string t_server,
			std::string t_port,
			std::string t_channel, // should start with '#'
			std::string t_nick,	  // all lower case
			std::string t_pass,    // should start with "oauth:"
			std::chrono::milliseconds t_write_delay
		);

		error_code_t connect() override;
		error_code_t login() override;
		error_code_t join_channel() override;
		error_code_t cap_req(const std::string& cap) override;
		std::pair<error_code_t, std::string> read() override;
		error_code_t write(const std::string& message) override;
		error_code_t reconnect() override;
		bool is_alive() const noexcept override;

	private:
		const std::string m_server;
		const std::string m_port;
		const std::string m_channel;
		const std::string m_nick;
		const std::string m_pass;

	public:
		static const std::string m_delimiter;
		const std::chrono::milliseconds m_write_delay{ 667 };

	protected:
		io_service_t m_io_service;

		resolver_t m_resolver{ m_io_service };
		socket_t m_socket{ m_io_service };
		streambuf_t m_buffer{};
	};

	class TwitchBot
	{
	public:
		TwitchBot(
			Commands&& t_commands,
			std::shared_ptr<IController> irc_controller,
			std::unique_ptr<message::MessageParser> t_parser
		);
		TwitchBot(TwitchBot&&) = default;
		TwitchBot& operator=(TwitchBot&&) = default;

		TwitchBot(const TwitchBot&) = delete;
		TwitchBot& operator=(const TwitchBot&) = delete;

		~TwitchBot() = default;

		void run();

	private:
		Commands m_commands;
		std::shared_ptr<IController> m_controller;
		std::unique_ptr<message::MessageParser> m_parser;
	};
}  // namespace Twitch::irc
#endif