#ifndef S_TO_U_H_
#define S_TO_U_H_

#include <limits>
#include <cstdint>

template<typename T>
T stou(const char *s){
	static_assert(std::is_unsigned<T>::value, "T must be unsigned");

	int const base = 10;

	auto const result = strtoull(s, nullptr, base);

	if (result > std::numeric_limits<T>::max())
		return 0;

	return static_cast<T>(result);
}

template<>
unsigned long      stou(const char *s){
	int const base = 10;
	return strtoul(s, nullptr, base);
}

template<>
unsigned long long stou(const char *s){
	int const base = 10;
	return strtoull(s, nullptr, base);
}

#endif

