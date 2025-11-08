#ifndef COMPARATOR_H_
#define COMPARATOR_H_

#include "sgn.h"

#include <type_traits>
#include <string_view>


namespace comparator{

namespace comparator_impl_{

	template <class T>
	int comp(const T &a, const T &b, std::false_type){
		if (a < b)
			return -1;

		if (a > b)
			return +1;

		return 0;
	}

	template <typename T>
	int comp(const T &a, const T &b, std::true_type){
		return sgn(a, b);
	}

}

// ===================================

template <typename T>
int comp(const T &a, const T &b){
	using namespace comparator_impl_;

	return comp(a, b, std::is_integral<T>{} );
}

constexpr int comp(std::string_view &a, std::string_view &b){
	using namespace comparator_impl_;

	return a.compare(b);
}

constexpr int comp(int const a, int const b){
	return a - b;
}

constexpr int comp(short int const a, short int const b){
	return a - b;
}

constexpr int comp(signed char const a, signed char const b){
	return a - b;
}


} // namespace

#endif

