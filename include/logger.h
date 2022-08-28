#ifndef MY_LOGGER_H_
#define MY_LOGGER_H_

#ifdef log__
	#error "log__ is a macro"
#endif

#include <cstdint>
#include <iostream>

namespace log_impl_{
	// MacOS have included "math.h" and log() is ambigous here.

	template<typename T>
	void logx(T const &t){
		std::clog << t << " ";
	}

	inline void logx(uint8_t const t){
		return logx<uint16_t>(t);
	}
}

namespace LogLevel{
	using type = int;

	constexpr type ZERO		= 0;
	constexpr type ERROR		= 1;
	constexpr type WARNING		= 2;
	constexpr type NOTICE		= 3;
	constexpr type DEBUG		= 4;
	constexpr type DEBUG_NOTICE	= 5;

	constexpr auto GlobalLogLevel = LogLevel::WARNING;
}



template<LogLevel::type level, typename... Args>
void log__(Args&&... args){
	if constexpr(level > LogLevel::GlobalLogLevel)
		return;

	using namespace log_impl_;

	(logx(args), ...);

	std::clog << '\n';
}

#endif

