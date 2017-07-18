#ifndef COMPARATOR_H_
#define COMPARATOR_H_

#include "sgn.h"

#include <type_traits>

#define log__(...)
//#include "logger.h"


namespace comparator{


template <class T>
int comp(const T &a, const T &b, std::false_type){
	log__("all");
	if (a < b)
		return -1;

	if (a > b)
		return +1;

	return 0;
}

// ===================================

template <typename T>
int comp(const T &a, const T &b, std::true_type){
	log__("sgn");
	return sgn(a, b);
}

// ===================================

template <typename T>
int comp(const T &a, const T &b){
	return comp(a, b, std::is_integral<T>{} );
}

template <>
int comp(int const &a, int const &b){
	log__("int");
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

