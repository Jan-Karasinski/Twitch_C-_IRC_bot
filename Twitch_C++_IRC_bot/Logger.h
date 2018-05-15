#ifndef LOGGER_H
#define LOGGER_H
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#include "TwitchMessage.h"
#include <boost\filesystem.hpp>
#include <fstream>
#include <ios>
#include <iostream>

// TODO: replace with Boost.Log
// TODO: initialize Boost.Log
namespace Logger {
	const boost::filesystem::path log_path{
		boost::filesystem::current_path().branch_path().append("log.txt")
	};

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