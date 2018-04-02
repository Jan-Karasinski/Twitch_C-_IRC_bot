#include "stdafx.h"
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#include "IRC_Bot.h"
#include <boost\algorithm\string.hpp>
#include <boost\algorithm\string\predicate.hpp>
#include <iostream>
#include <thread>
#include <regex>
#include <optional>

namespace Twitch::IRC {
	Controller::Controller(
		const std::string& t_server,
		const std::string& t_port,
		const std::string& t_channel,
		const std::string& t_nick,
		const std::string& t_pass,
		const std::chrono::milliseconds& t_write_delay
	) :
		m_server(t_server),
		m_port(t_port),
		m_channel(t_channel),
		m_nick(t_nick),
		m_pass(t_pass),
		m_write_delay(t_write_delay)
	{
	}

	error_code_t Controller::connect() {
		error_code_t error{};

		auto endpoint_it{
			resolver.resolve(
				resolver_t::query{ m_server, m_port },
				error
			)
		};
		if (error) return error;

		std::cout << "Connecting... ";

		boost::asio::connect(m_socket, endpoint_it, error);

		if (!error) std::cout << "Connected!\n";
		else std::cout << "Failed\n";

		return error;
	}

	error_code_t Controller::login() {
		error_code_t error{};

		using namespace std::string_literals;
		error = this->write("PASS :"s + m_pass);
		if (error) { return error; }

		return this->write("NICK :"s + m_nick);
	}

	error_code_t Controller::join_channel() {
		using namespace std::string_literals;
		return this->write("JOIN :"s + this->m_channel);
	}

	error_code_t Controller::cap(const std::string& cap) {
		using namespace std::string_literals;
		return this->write("CAP REQ "s + cap);
	}

	error_code_t Controller::read() {
		error_code_t error{};

		std::cout << "Reading... ";
		boost::asio::read_until(
			m_socket,
			m_buffer,
			m_delimiter,
			error
		);
		if (!error) std::cout << "Success\n";
		else std::cout << "Fail\n";

		return error;
	}

	error_code_t Controller::write(const std::string& message) {
		error_code_t error{};

		std::cout << "Writing... ";
		boost::asio::write(
			m_socket,
			boost::asio::buffer(message + m_delimiter),
			error
		);
		if (!error) std::cout << "Success\n";
		else std::cout << "Fail\n";

		return error;
	}

	bool Controller::is_alive() const noexcept {
		return m_socket.is_open();
	}

	const std::string Controller::m_delimiter{ "\r\n" };

	TwitchBot::TwitchBot(
		std::shared_ptr<IController> irc_controller
	) :
		m_controller(irc_controller)
	{
	}

	void TwitchBot::run() {
		if (auto error = m_controller->connect();  error) {
			std::cerr << error.message() << '\n';
			return;
		}

		using namespace std::string_literals;
		if (auto error = m_controller->cap(":twitch.tv/tags"s); error) {
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
			if (auto error = m_controller->read(); error) {
				std::cerr << error.message() << '\n';
				return;
			}

			std::string recived_message;
			std::getline(std::istream(&m_controller->m_buffer), recived_message);

			if (boost::starts_with(recived_message, "PING")) {
				using namespace std::string_literals;
				m_controller->write("PONG :tmi.twitch.tv"s);
			}
			std::cout << recived_message << '\n';
			
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(10ms);
		}
	}
}