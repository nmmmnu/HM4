#ifndef COMPARATOR_H_
#define COMPARATOR_H_

#include "sgn.h"

#define log__(...)
//#include "logger.h"

namespace comparator{

template <class T>
int comp(const T &a, const T &b){
	log__("all");
	if (a < b)
		return -1;

	if (a > b)
		return +1;

	return 0;
}

inline int comp(int const a, int const b){
	log__("int");
	return a - b;
}

inline int comp(long int const a, long int const b){
	log__("long");
	return sgn(a - b);
}

} // namespace

#endif

