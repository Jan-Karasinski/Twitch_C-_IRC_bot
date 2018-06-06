#include "stdafx.h"
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#include "IRC_Bot.h"
#include "TwitchMessage.h"
#include "Logger.h"
#include <boost\algorithm\string.hpp>
#include <boost\algorithm\string\predicate.hpp>
#include <iostream>
#include <thread>
#include <regex>
#include <optional>

namespace Twitch::irc {
	void MessageQueue::push(std::string message, bool priority) {
		std::unique_lock<std::mutex> lock{ m_mutex };
		MultithreadingRoutine routine(
			lock, m_cv, m_ready_for_task,
			[&]() -> bool { return m_ready_for_task; }
		);

		try {
			if (priority) { m_queue.emplace_front(std::move(message)); }
			else { m_queue.emplace_back(std::move(message)); }
		} catch (...) {}
	}

	std::optional<std::string> MessageQueue::pop() {
		std::unique_lock<std::mutex> lock{ m_mutex };
		MultithreadingRoutine routine(
			lock, m_cv,
			m_ready_for_task,
			[&]() { return !m_queue.empty() && m_ready_for_task; }
		);

		try {
			if (m_queue.empty()) { return std::nullopt; }

			std::string value = std::move(m_queue.front());
			m_queue.pop_front();
			return std::move(value);
		} catch (...) {
			return std::nullopt;
		}
	}

	using namespace std::string_literals;
	const std::string Commands::cmd_indicator{ "!"s };

	size_t Commands::min_cmd_word_size() noexcept {
		return cmd_indicator.size() + 3;
	}

	Commands::cmd_handle_t Commands::find(const std::string& key) const {
		std::lock_guard<std::mutex> lock{ m_mutex };
		const auto pos = m_commands.find(key);

		if (pos == m_commands.end()) { return nullptr; }

		return pos->second;
	}

	Commands::Commands(std::initializer_list<value_type> init)
		: m_commands(init)
	{
	}

	Commands::Commands(const Commands& c)
		: m_commands(c.m_commands)
	{
	}

	Commands& Commands::operator=(const Commands& c) {
		m_commands = c.m_commands;
		return *this;
	}

	WritingThread::WritingThread(std::shared_ptr<IRCWriter> t_writer)
		: m_writer(t_writer),
		m_writing_thread(
			[&](std::shared_ptr<IRCWriter> writer) {
				while (!m_terminate) {
					if (auto response = writer->get_message_queue()->pop()) {
						writer->write(response.value());
					}
				}
			},
			m_writer
		)
	{
	}
	
	WritingThread::~WritingThread()
	{
		m_terminate = true;
		m_writing_thread.join();
	}

	Controller::Controller(
		std::string t_server,
		std::string t_port,
		std::string t_channel,
		std::string t_nick,
		std::string t_pass,
		std::chrono::milliseconds t_write_delay
	) :
		m_server(std::move(t_server)),
		m_port(std::move(t_port)),
		m_channel(std::move(t_channel)),
		m_nick(std::move(t_nick)),
		m_pass(std::move(t_pass)),
		m_write_delay(t_write_delay)
	{
	}

	error_code_t Controller::connect() {
		error_code_t error{};

		auto endpoint_it{
			m_resolver.resolve(
				resolver_t::query{ m_server, m_port },
				error
			)
		};
		if (error) { return error; }

		std::cout << "Connecting... ";

		boost::asio::connect(m_socket, endpoint_it, error);

		if (!error) { std::cout << "Connected!\n"; }
		else        { std::cout << "Failed\n"; }

		return error;
	}

	error_code_t Controller::login() {
		using namespace std::string_literals;
		if (const auto error = this->write("PASS :"s + m_pass); error) {
			return error;
		}
		return this->write("NICK :"s + m_nick);
	}

	error_code_t Controller::join_channel() {
		using namespace std::string_literals;
		return this->write("JOIN :"s + this->m_channel);
	}

	error_code_t Controller::cap_req(const std::string& cap) {
		using namespace std::string_literals;
		return this->write("CAP REQ "s + cap);
	}

	std::pair<error_code_t, std::string> Controller::read() {
		error_code_t error{};

		std::size_t n = boost::asio::read_until(
			m_socket,
			m_buffer,
			m_delimiter,
			error
		);

		if (error) { return { error, {} }; }
		
		std::string recived_message;
		std::getline(std::istream(&m_buffer), recived_message);
		
		return { error, std::move(recived_message) };
	}

	error_code_t Controller::write(const std::string& message) {
		std::unique_lock<std::mutex> lock{ m_mutex };
		m_cv.wait(lock, [&]() { return m_ready_to_write && !m_reconnecting; });

		error_code_t error{};

		boost::asio::write(
			m_socket,
			boost::asio::buffer(message + m_delimiter),
			error
		);

		if (!error) { std::this_thread::sleep_for(m_write_delay); }

		return error;
	}

	void Controller::enqueue(std::string message, bool priority) {
		m_queue->push(std::move(message), priority);
	}

	error_code_t Controller::reconnect() {
		std::lock_guard lock(m_mutex);
		m_reconnecting = true;
		m_socket = socket_t{ m_io_service };

		if (auto error = connect(); error) { return error; }
		using namespace std::string_literals;
		if (auto error = cap_req(":twitch.tv/tags twitch.tv/commands twitch.tv/membership"s);
			error) { return error; }
		if (auto error = login(); error) { return error; }
		if (auto error = join_channel(); error) { return error; }

		m_reconnecting = false;
		return {};
	}

	bool Controller::is_alive() const noexcept {
		return m_socket.is_open();
	}

	std::shared_ptr<MessageQueue> Controller::get_message_queue() {
		return m_queue;
	}

	const std::string Controller::m_delimiter{ "\r\n" };

	TwitchBot::TwitchBot(
		std::shared_ptr<Commands> t_commands,
		std::shared_ptr<IController> irc_controller,
		std::unique_ptr<message::MessageParser> t_parser
	) :
		m_commands(std::move(t_commands)),
		m_controller(std::move(irc_controller)),
		m_parser(std::move(t_parser))
	{
	}

	void TwitchBot::run() {
		if (auto error = m_controller->connect(); error) {
			std::cerr << error.message() << '\n';
			return;
		}

		using namespace std::string_literals;
		if (auto error = m_controller->cap_req(
				":twitch.tv/tags twitch.tv/commands twitch.tv/membership"s
			); error) {
			std::cerr << error.message() << '\n';
			return;
		}

		if (auto error = m_controller->login(); error) {
			std::cerr << error.message() << '\n';
			return;
		}

		if (auto error = m_controller->join_channel(); error) {
			std::cerr << error.message() << '\n';
			return;
		}

		WritingThread writing_thread(m_controller);
		
		while (m_controller->is_alive()) {
			auto [error, recived_message] = m_controller->read();
			if (error) {
				std::cerr << error.message() << '\n';
				return;
			}
#ifdef TWITCH_IRC_COLLECT_SAMPLES
			using severity = boost::log::trivial::severity_level;
			BOOST_LOG_SEV(m_lg, severity::trace)
				<< " UNPROCESSED: " << recived_message;
#endif
			auto parse_result = m_parser->process(recived_message);
			boost::apply_visitor(
				m_parser->get_visitor(m_controller, m_commands, m_lg),
				parse_result
			);

			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1ms);
		}
	}
}