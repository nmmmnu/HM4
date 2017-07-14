#ifndef SGN_H_
#define SGN_H_

#include <type_traits>

template<typename T>
constexpr int sgn(const T &a) noexcept{
	static_assert(std::is_signed<T>::value, "T must be signed type");
	return (T(0) < a) - (a < T(0));
}

#endif

