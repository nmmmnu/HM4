#ifndef S_TO_U_SAFE_H_
#define S_TO_U_SAFE_H_

#include "stringref.h"

#include <sstream>

template <typename T>
static T stou(const StringRef &str, T const def = 0){
	static_assert(std::is_integral<T>::value, "T must be integral type");

	if (str.empty())
		return def;

	T u = 0;
	std::istringstream ss(str);
	ss >> u;
	return u;
}

#endif

