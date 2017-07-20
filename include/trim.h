#ifndef MY_TRIM_H
#define MY_TRIM_H

#include <string>
#include <cstring>
#include <cassert>

namespace trim_impl{
	constexpr const char *TRIM_CHARACTERS = " \t\r\n";

	inline bool isspace_(char const c){
		return	c == TRIM_CHARACTERS[0] ||
			c == TRIM_CHARACTERS[1] ||
			c == TRIM_CHARACTERS[2] ||
			c == TRIM_CHARACTERS[3];
	}
} // trim_impl

inline std::string &trim(std::string &line){
	using namespace trim_impl;

	line.erase(line.find_last_not_of(TRIM_CHARACTERS) + 1);
	return line;
}

inline size_t trim_size(char *s, size_t const size){
	assert(s);

	using namespace trim_impl;

	char *c = s + size;

	while(--c >= s)
		if ( isspace_(*c) )
			*c = '\0';
		else
			break;

	return size_t(c - s) + 1;
}

inline char *trim(char *s, size_t const size){
	trim_size(s, size);
	return s;
}

inline char *trim(char *s){
	return trim(s, strlen(s));
}

#endif

