#include <iostream>

#ifndef MY_LOGGER_H_
#define MY_LOGGER_H_

	#ifdef log__
		#undef log__
	#endif


	template<typename... Args>
	void log__(Args... args){
		((std::clog << args << ' '), ...);

		std::clog << '\n';
	}

#endif

