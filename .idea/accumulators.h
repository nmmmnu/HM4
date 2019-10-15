#ifndef ACCUMULATORS_H_
#define ACCUMULATORS_H_

#include <limits>
#include <cstdint>
#include <functional>



template<typename T, class F>
class CountAcummulator{
	size_t count = 0;

	F f;

public:
	constexpr CountAcummulator(F f) : f(std::move(f)){}

	constexpr void operator()(T const &a){
		if (std::invoke(f, a))
			++count;
	}

	constexpr size_t get() const{
		return count;
	}
};



template<typename T, class F, T limit = T{0}>
class CompareAcummulator{
	T val = limit;

	F f;

public:
	constexpr CompareAcummulator(F f) : f(std::move(f)){}

	constexpr void operator()(T const &a){
		if (f(a, val))
			val = a;
	}

	constexpr size_t get() const{
		return val;
	}
};



#endif

