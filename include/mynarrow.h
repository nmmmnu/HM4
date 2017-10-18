#ifndef MY_NARROW_H_
#define MY_NARROW_H_

#include <utility>	// is_arithmetic
#include <typeinfo>	// bad_cast
#include <limits>	// numeric_limits

template<class T, class U>
constexpr T narrow(U const u){
	static_assert(std::is_arithmetic<T>::value, "T must be arithmetic");
	static_assert(std::is_arithmetic<U>::value, "U must be arithmetic");

	if (std::is_same<T, U>::value)
		return u;

	T const t = static_cast<T>(u);

	if (static_cast<U>(t) != u)
		throw std::bad_cast();

	if ( std::numeric_limits<T>::is_signed != std::numeric_limits<U>::is_signed ){
		if (std::numeric_limits<T>::is_signed && t < 0)
			throw std::bad_cast();

		if (std::numeric_limits<U>::is_signed && u < 0)
			throw std::bad_cast();
	}

	return t;
}

#endif

