#ifndef LOGGER_H
#define LOGGER_H
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#include <boost\filesystem.hpp>
#include <fstream>
#include <ios>
#include <iostream>

namespace Logger {
	const boost::filesystem::path log_path{
		boost::filesystem::current_path().branch_path().append("log.txt")
	};

	struct DummyLogger
	{
		inline static DummyLogger get() noexcept { return {}; };

		template<typename T>
		DummyLogger& operator<<(T&& /*unused*/) {
			return *this;
		}
	};
	
	class DefaultLogger
	{
		std::ofstream log{ log_path.string(), std::ios::app };

	public:
		inline static DefaultLogger get() noexcept { return {}; };

		template<typename T>
		DefaultLogger& operator<<(T&& t) {
			std::cout << t;
			log << t;
			return *this;
		}
	};

} // namespace Logger
#endif // LOGGER_H