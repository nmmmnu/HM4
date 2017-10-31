#ifndef MY_NARROW_H_
#define MY_NARROW_H_

#include <utility>	// is_arithmetic
#include <typeinfo>	// bad_cast
#include <limits>	// numeric_limits
#include <type_traits>

namespace narrow_impl_{
	
	template<class T, class U>
	constexpr T narrow_(U const u, std::false_type){
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
	
	template<class T>
	constexpr T narrow_(T const t, std::true_type){
		return t;
	}
}

template<class T, class U>
constexpr T narrow(U const u){
	return narrow_impl_::narrow_<T>(u, std::is_same<T, U>{} );
}

#endif

