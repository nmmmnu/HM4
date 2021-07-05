#ifndef BFS_LOOKUP_H
#define BFS_LOOKUP_H


#include <cstdint>
#include <cassert>
#include <array>

#include "fixedvector.h"

template<typename T, T Levels>
class BFSLookupFactory{
private:
	constexpr void reorder_(){
		for(T level = 0; level < Levels; ++level)
			reorder_(level);
	}

	constexpr void reorder_(T const level){
		reorder_(0, SIZE, level, 0);
	}

	constexpr void reorder_(T const begin, T const end, T const target_level, T const current_level){
		if (begin >= end){
			// size is guaranteed to be "aligned",
			// no push_(NIL) needed...
			return;
		}

		T const mid = T( begin + ((end - begin) >> 1) );

		if (current_level == target_level)
			return value.push_back(mid);

		if (current_level < target_level){
			T const next_level = T(current_level + 1);

			reorder_(begin,      mid, target_level, next_level);
			reorder_(T(mid + 1), end, target_level, next_level);
		}
	}

public:
	constexpr static auto build(){
		BFSLookupFactory f;
		f.reorder_();

		std::array<T, SIZE> a{};

		// std::copy
		for(typename FixedVector<T, SIZE>::size_type i = 0; i < f.value.size(); ++i)
			a[i] = f.value[i];

		return a;
	}

private:
	constexpr static T SIZE = (1 << Levels) - 1;

	FixedVector<T, SIZE> value;
};

#endif

