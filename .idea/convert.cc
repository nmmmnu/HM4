#include <iostream>

#include <cstdlib>
#include <limits>
#include <cstdint>


template<typename T>
T stou(const char *s, int const base = 10){
	static_assert(std::is_unsigned<T>::value, "T must be unsigned");

	auto const result = strtoull(s, nullptr, base);

	if (result > std::numeric_limits<T>::max())
		return 0;

	return static_cast<T>(result);
}

template<>
unsigned long      stou(const char *s, int const base){
	return strtoul(s, nullptr, base);
}

template<>
unsigned long long stou(const char *s, int const base){
	return strtoull(s, nullptr, base);
}


int main(){
	std::cout << (int) stou<uint8_t>("-12") << '\n';
	std::cout << (int) stou<uint8_t>("123") << '\n';
	std::cout << (int) stou<uint8_t>("455") << '\n';
	std::cout << stou<uint16_t>("123") << '\n';
	std::cout << stou<uint32_t>("123") << '\n';
	std::cout << stou<uint64_t>("123") << '\n';
	std::cout << stou<uint64_t>("-12") << '\n';
}

