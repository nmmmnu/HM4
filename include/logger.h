#include <iostream>

#ifndef MY_LOGGER_H_
#define MY_LOGGER_H_

	#ifdef log__
		#undef log__
	#endif


	inline void log__(){
		std::clog << '\n';
	}

	template<typename T, typename... ARGS>
	void log__(T first, ARGS... args){
		std::clog << first << " ";
		log__(args...);
	}

#endif

