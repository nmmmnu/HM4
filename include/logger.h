#include <iostream>

#ifdef log__
#undef log__
#endif

#ifndef MY_LOGGER_H_
#define MY_LOGGER_H_

namespace{
	inline void log__(){
		std::clog << '\n';
	}

	template<typename T, typename... ARGS>
	void log__(T first, ARGS... args){
		std::clog << first << " ";
		log__(args...);
	}
}

#endif

