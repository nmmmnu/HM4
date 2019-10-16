#ifndef MY_TYPE_TRAITS_H_
#define MY_TYPE_TRAITS_H_

// Check for C++17
#if __cplusplus < 201703L

namespace std{

	template<class ...>
	using void_t = void;

	template <bool B>
	using bool_constant = integral_constant<bool, B>;

}

#else

#include <type_traits>

#endif

#endif
