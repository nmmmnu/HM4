#ifndef S_TO_U_SAFE_H_
#define S_TO_U_SAFE_H_

#include "stringref.h"

#include <sstream>

namespace impl_{
	template <typename T>
	void ston_safe_check(){
		static_assert(std::is_integral<T>::value, "T must be integral type");

		static_assert(! std::is_same<T,          char>::value, "T must not be char type");
		static_assert(! std::is_same<T,   signed char>::value, "T must not be char type");
		static_assert(! std::is_same<T, unsigned char>::value, "T must not be char type");
	}
} // namespace

template <typename T>
T ston_safe(const StringRef &str, T const def = 0){
	impl_::ston_safe_check<T>();

	if (str.empty())
		return def;

	T u = 0;
	std::istringstream ss(str);
	ss >> u;
	return u;
}

template <typename T>
std::string ntos_safe(T const n){
	impl_::ston_safe_check<T>();

	std::stringstream ss;
	ss << n;
	return ss.str();
}

#endif

