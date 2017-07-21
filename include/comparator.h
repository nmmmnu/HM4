#ifndef COMPARATOR_H_
#define COMPARATOR_H_

#include "sgn.h"

#include <type_traits>


namespace comparator{


template <class T>
int comp(const T &a, const T &b, std::false_type){
	if (a < b)
		return -1;

	if (a > b)
		return +1;

	return 0;
}

// ===================================

template <typename T>
int comp(const T &a, const T &b, std::true_type){
	return sgn(a, b);
}

// ===================================

template <typename T>
int comp(const T &a, const T &b){
	return comp(a, b, std::is_integral<T>{} );
}

template <>
int comp(int const &a, int const &b){
	return a - b;
}

template <>
int comp(short int const &a, short int const &b){
	return comp<int>(a, b);
}

template <>
int comp(signed char const &a, signed char const &b){
	return comp<int>(a, b);
}


} // namespace

#endif

