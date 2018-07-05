#ifndef MY_NARROW_H_
#define MY_NARROW_H_

#include <utility>	// is_arithmetic
#include <typeinfo>	// bad_cast
#include <limits>	// numeric_limits
#include <type_traits>
#include "my_void_t.h"

namespace mynarrow_impl_{

	template<typename T, typename U>
	constexpr T narrow_(U const u, std::false_type){
		static_assert(! std::is_same<T, U>::value,  "T and U must be different types");

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

	template<typename T, typename U>
	constexpr T narrow_(U const u, std::true_type) noexcept{
		return u;
	}

	// ========================

	template <class T, class U, class = void>
	struct can_hold : std::false_type{};

	template <class T, class U>
	struct can_hold<T, U, my_void_t<decltype( T{ std::declval<U>() } )> > : std::true_type{};
} // namespace mynarrow_impl_


template<class T, class U>
constexpr T narrow(U const u){
	static_assert(std::is_arithmetic<T>::value, "T must be arithmetic");
	static_assert(std::is_arithmetic<U>::value, "U must be arithmetic");

	return mynarrow_impl_::narrow_<T>(u, mynarrow_impl_::can_hold<T, U>{} );
}

#endif

