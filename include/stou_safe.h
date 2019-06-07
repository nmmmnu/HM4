#ifndef S_TO_U_SAFE_H_
#define S_TO_U_SAFE_H_

#include "stringref.h"

#include <sstream>

template <typename T>
T stou_safe(const StringRef &str, T const def = 0){
	static_assert(std::is_integral<T>::value, "T must be integral type");

	static_assert(! std::is_same<T,   signed char>::value, "T must not be char type");
	static_assert(! std::is_same<T, unsigned char>::value, "T must not be char type");

	if (str.empty())
		return def;

	T u = 0;
	std::istringstream ss(str);
	ss >> u;
	return u;
}

template <typename T>
std::string utos_safe(T const n){
	static_assert(std::is_integral<T>::value, "T must be integral type");

	static_assert(! std::is_same<T,   signed char>::value, "T must not be char type");
	static_assert(! std::is_same<T, unsigned char>::value, "T must not be char type");

	std::stringstream ss;
	ss << n;
	return ss.str();
}

#endif

