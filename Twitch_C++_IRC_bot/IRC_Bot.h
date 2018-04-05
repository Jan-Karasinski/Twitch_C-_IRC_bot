#ifndef IRC_BOT_H
#define IRC_BOT_H
#include "TwitchMessage.h"
#include <WinSock2.h>
#include <boost\asio.hpp>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <chrono>

namespace Twitch::IRC {
	using io_service_t = boost::asio::io_service;
	using resolver_t   = boost::asio::ip::tcp::resolver;
	using socket_t     = boost::asio::ip::tcp::socket;
	using streambuf_t  = boost::asio::streambuf;
	using error_code_t = boost::system::error_code;
	
	struct IRCReader
	{
		virtual error_code_t read() = 0;
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
		virtual error_code_t cap(const std::string& cap) = 0;
		virtual bool is_alive() const noexcept = 0;
		virtual ~IController() = default;

	private:
		io_service_t io_service;

	public:
		resolver_t resolver{ io_service };
		socket_t m_socket{ io_service };
		streambuf_t m_buffer;
	};

	class Controller : public IController
	{ /// https://dev.twitch.tv/docs/irc#connecting-to-twitch-irc
	public:
		Controller(
			const std::string& t_server,
			const std::string& t_port,
			const std::string& t_channel, // should start with '#'
			const std::string& t_nick,	  // all lower case
			const std::string& t_pass,    // should start with "oauth:"
			const std::chrono::milliseconds& t_write_delay
		);

		error_code_t connect() override;
		error_code_t login() override;
		error_code_t join_channel() override;
		error_code_t cap(const std::string& cap) override;
		virtual error_code_t read() override;
		virtual error_code_t write(const std::string& message) override;
		virtual bool is_alive() const noexcept override;

	public:
		static const std::string m_delimiter;
		const std::chrono::milliseconds m_write_delay{ 667 };

	private:
		const std::string m_server;
		const std::string m_port;
		const std::string m_channel;
		const std::string m_nick;
		const std::string m_pass;
	};

	class TwitchBot
	{
	public:
		explicit TwitchBot(
			std::shared_ptr<IController> irc_controller,
			std::unique_ptr<Message::MessageParser> t_parser
		);
		TwitchBot(TwitchBot&&) = default;
		TwitchBot& operator=(TwitchBot&&) = default;

		TwitchBot(const TwitchBot&) = delete;
		TwitchBot& operator=(const TwitchBot&) = delete;

		~TwitchBot() = default;

		void run();

	private:
		std::shared_ptr<IController> m_controller;
		std::unique_ptr<Message::MessageParser> m_parser;
	};
}  // namespace Twitch::IRC
#endif