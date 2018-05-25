#ifndef LOGGER_H
#define LOGGER_H
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#include "TwitchMessage.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <ios>
#include <iostream>

// TODO: replace with Boost.Log
// TODO: initialize Boost.Log
namespace Logger {
	const boost::filesystem::path log_path{
		boost::filesystem::current_path().branch_path().append("log.txt")
	};

	inline void init() {
		boost::log::add_file_log(
			boost::log::keywords::file_name = log_path,
			boost::log::keywords::open_mode = std::ios_base::app,
			boost::log::keywords::format = "[%TimeStamp%] (%LineID%) <%Severity%>: %Message%",
			boost::log::keywords::auto_flush = true
		);
		boost::log::add_console_log(
			std::cout,
			boost::log::keywords::format = "[%TimeStamp%] (%LineID%) <%Severity%>: %Message%"
		);
		
		boost::log::add_common_attributes();
		boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");
	}

	struct DummyLogger
	{
		inline static DummyLogger get() noexcept { return {}; };

		template<typename T>
		DummyLogger& operator<<([[maybe_unused]] T&& t) {
			return *this;
		}
	};
	
	class DefaultLogger
	{
		std::ofstream log{ log_path.string(), std::ios::app };
		DefaultLogger() = default;

	public:
		inline static DefaultLogger get() noexcept { return {}; };

		DefaultLogger(DefaultLogger&&) = default;
		DefaultLogger& operator=(DefaultLogger&&) = default;

		template<typename T>
		DefaultLogger& operator<<(T&& t) {
			std::cout << t;
			log << t;
			return *this;
		}
	};

} // namespace Logger
#endif // LOGGER_H