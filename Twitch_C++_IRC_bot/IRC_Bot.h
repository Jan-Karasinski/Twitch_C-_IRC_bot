#ifndef IRC_BOT_H
#define IRC_BOT_H
#include "Logger.h"
#include <WinSock2.h>
#include <boost\asio.hpp>
#include <chrono>
#include <memory>
#include <map>
#include <string>
#include <vector>
#include <optional>
#include <deque>
#include <mutex>
#include <condition_variable>

namespace Twitch::irc {
	namespace message {
		class MessageParser;
		namespace cap::tags {
			struct PRIVMSG;
		}
	}

	// RAII blessing for thread synchronization
	struct MultithreadingRoutine final
	{
	public:
		template<class Pred>
		MultithreadingRoutine(
			std::unique_lock<std::mutex>& t_lock,
			std::condition_variable& t_cv,
			std::atomic_bool& t_control_value,
			Pred cv_predicate
		) :
			m_lock(t_lock), m_cv(t_cv), m_control_value(t_control_value)
		{
			t_cv.wait(m_lock, cv_predicate);
			m_control_value = false;
		}

		~MultithreadingRoutine() {
			m_control_value = true;
			m_lock.unlock();
			m_cv.notify_all();
		}

	private:
		std::unique_lock<std::mutex>& m_lock;
		std::condition_variable& m_cv;
		std::atomic_bool& m_control_value;
	};

	// threadsafe, all public ops are sync'd
	class MessageQueue
	{
	public:
		void push(std::string message, bool priority = false);

		std::optional<std::string> pop();

	private:
		std::deque<std::string> m_queue;

		mutable std::mutex m_mutex{};
		std::condition_variable m_cv{};
		mutable std::atomic_bool m_ready_for_task{ true };
	};

	using io_service_t = boost::asio::io_service;
	using resolver_t   = boost::asio::ip::tcp::resolver;
	using socket_t     = boost::asio::ip::tcp::socket;
	using streambuf_t  = boost::asio::streambuf;
	using error_code_t = boost::system::error_code;
	using logger_t     = boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level>;

	struct Commands
	{ // TODO: thread safety
		using key_type = std::string;
		using cmd_handle_t = std::function<std::string(const message::cap::tags::PRIVMSG &)>;
		using value_type = std::map<std::string, cmd_handle_t>::value_type;
		
		static const std::string cmd_indicator; // symbol to distinct commands from regular messages, usually '!'
		static size_t min_cmd_word_size() noexcept; // min size of word used as command name, e.g. "!uptime"

		cmd_handle_t find(const std::string& key) const;

		Commands(std::initializer_list<value_type> init);
		Commands(const Commands& c);
		Commands& operator=(const Commands& c);

	private:
		std::map<std::string, cmd_handle_t> m_commands;
		mutable std::mutex m_mutex;
	};

	struct IRCWriter;
	class WritingThread
	{
	public:
		WritingThread(std::shared_ptr<IRCWriter> t_writer);
		~WritingThread();

	private:
		std::shared_ptr<IRCWriter> m_writer;
		std::thread m_writing_thread;
		bool m_terminate{ false };
	};

	struct IRCReader
	{
		virtual std::pair<error_code_t, std::string> read() = 0;
		virtual ~IRCReader() = default;
	};

	struct IRCWriter
	{
		virtual error_code_t write(const std::string&) = 0;
		virtual void enqueue(std::string message, bool priority = false) = 0;
		virtual std::shared_ptr<MessageQueue> get_message_queue() = 0;
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
			std::string t_nick,	   // all lower case
			std::string t_pass,    // should start with "oauth:"
			std::chrono::milliseconds t_write_delay
		);

		error_code_t connect() override;
		error_code_t login() override;
		error_code_t join_channel() override;
		error_code_t cap_req(const std::string& cap) override;
		std::pair<error_code_t, std::string> read() override;
		error_code_t write(const std::string& message) override;
		void enqueue(std::string message, bool priority) override;
		error_code_t reconnect() override;
		bool is_alive() const noexcept override;
		std::shared_ptr<MessageQueue> get_message_queue() override;

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
		std::shared_ptr<MessageQueue> m_queue{ std::make_shared<MessageQueue>() };

		mutable std::mutex m_mutex;
		mutable std::condition_variable m_cv;
		mutable std::atomic_bool m_ready_to_write{ true };
		std::atomic_bool m_reconnecting{ false };
	};

	class TwitchBot
	{
	public:
		TwitchBot(
			std::shared_ptr<Commands> t_commands,
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
		std::shared_ptr<Commands> m_commands;
		std::shared_ptr<IController> m_controller;
		std::unique_ptr<message::MessageParser> m_parser;

		mutable logger_t m_lg{};
	};
}  // namespace Twitch::irc
#endif