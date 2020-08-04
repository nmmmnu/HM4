#ifndef MY_LOGGER_H_
#define MY_LOGGER_H_

	#ifdef log__
		#undef log__
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

	template<typename... Args>
	void log__(Args&&... args){
		using namespace log_impl_;

		(logx(args), ...);

		std::clog << '\n';
	}

#endif

