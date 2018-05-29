#ifndef LOGGER_H
#define LOGGER_H
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/filesystem.hpp>
#include <ios>
#include <iostream>

namespace Logger {
	const boost::filesystem::path log_path{
		boost::filesystem::current_path().branch_path().append("log.txt")
	};

	inline void init() {
		boost::log::add_common_attributes();
		boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");

		boost::log::add_file_log(
			boost::log::keywords::file_name = log_path,
			boost::log::keywords::open_mode = std::ios_base::app,
			boost::log::keywords::format = "[%TimeStamp%] (%LineID%) <%Severity%>: %Message%",
			boost::log::keywords::auto_flush = true
		);
		boost::log::add_console_log(
			std::cout,
			boost::log::keywords::format = "[%TimeStamp%] (%LineID%) <%Severity%>: %Message%",
			boost::log::keywords::auto_flush = true
		);
	}
} // namespace Logger
#endif // LOGGER_H