#include <cstdint>
#include <cstring>

template<typename T>
T oph_(const char *s){
	constexpr std::size_t MAX = sizeof(T);
	const     std::size_t size = strnlen(s, MAX);

	T r = 0;

	for(auto it = s; it - s < size; ++it)
		r = r << 8 | *it;

	return r;
}

inline uint64_t oph(const char *s){
	return oph_<uint64_t>(s);
}

#include <iostream>

int main(){
	uint64_t const a = oph("New York City");
	uint64_t const b = oph("Boston International");

	std::cout << (a > b ? "a" : "b") << '\n';
	std::cout << (a < b ? "a" : "b") << '\n';
}

