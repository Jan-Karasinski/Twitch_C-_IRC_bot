#ifndef LOGGER_H
#define LOGGER_H
#include <iostream>
#include <fstream>

class PrimitiveLogger
{
	static std::ofstream log;

public:
	inline static PrimitiveLogger get() noexcept { return {}; };

	template<typename T>
	static std::ofstream operator<<(T&& t) {
		std::cout << t;
		return log << t;
	}
};

#endif // !LOGGER_H