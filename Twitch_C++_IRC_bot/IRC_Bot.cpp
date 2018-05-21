#include "stdafx.h"
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#include "IRC_Bot.h"
#include "Logger.h"
#include <boost\algorithm\string.hpp>
#include <boost\algorithm\string\predicate.hpp>
#include <iostream>
#include <thread>
#include <regex>
#include <optional>

namespace Twitch::irc {
	using namespace std::string_literals;
	const std::string Commands::cmd_indicator{ "!"s };

	Commands::cmd_handle_t Commands::find(const std::string & key) const {
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
		error_code_t error{};

		boost::asio::write(
			m_socket,
			boost::asio::buffer(message + m_delimiter),
			error
		);

		return error;
	}

	error_code_t Controller::reconnect() {
		m_socket = socket_t{ m_io_service };

		if (auto error = connect(); error) { return error; }
		using namespace std::string_literals;
		if (auto error = cap_req(":twitch.tv/tags twitch.tv/commands twitch.tv/membership"s);
			error) { return error; }
		if (auto error = login(); error) { return error; }
		if (auto error = join_channel(); error) { return error; }
		return {};
	}

	bool Controller::is_alive() const noexcept {
		return m_socket.is_open();
	}

	const std::string Controller::m_delimiter{ "\r\n" };

	TwitchBot::TwitchBot(
		Commands&& t_commands,
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

		while (m_controller->is_alive()) {
			auto [error, recived_message] = m_controller->read();
			if (error) {
				std::cerr << error.message() << '\n';
				return;
			}
			
			auto parse_result = m_parser->process(recived_message);
			boost::apply_visitor(
				m_parser->get_visitor(&m_commands),
				parse_result
			);

			using namespace std::chrono_literals;
			std::this_thread::sleep_for(5ms);
		}
	}
}