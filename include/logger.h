#ifndef MY_LOGGER_H_
#define MY_LOGGER_H_

	#ifdef log__
		#undef log__
	#endif

	#include <cstdint>
	#include <iostream>

	namespace log_impl_{
		template<typename T>
		void log(T const &t){
			std::clog << t << " ";
		}

		inline void log(uint8_t const t){
			return log<uint16_t>(t);
		}
	}

	template<typename... ARGS>
	void log__(ARGS... args){
		using namespace log_impl_;

		(log(args), ...);

		std::clog << '\n';
	}

#endif

