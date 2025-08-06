#ifndef MY_HAMMING_H_
#define MY_HAMMING_H_

#include <cstdint>
#include <array>
#include <algorithm>
#include "staticvector.h"

namespace hamming1_impl_{

	template<typename T>
	constexpr void sort9(std::array<T, 9> &m){

		auto swap = [&m](size_t ia, size_t ib){
			auto const a = std::min(m[ia], m[ib]);
			auto const b = std::max(m[ia], m[ib]);
			m[ia] = a;
			m[ib] = b;
		};

		// sorting network 9
		swap(0,1); swap(3,4); swap(6,7);
		swap(1,2); swap(4,5); swap(7,8);
		swap(0,1); swap(3,4); swap(6,7);
		swap(0,3); swap(3,6); swap(0,3);
		swap(1,4); swap(4,7); swap(1,4);
		swap(5,8); swap(2,5); swap(5,8);
		swap(2,4); swap(4,6); swap(2,4);
		swap(1,3); swap(2,3);
		swap(5,7); swap(5,6);
	}

} // namespace hamming1_impl_

template<bool sort = 0>
constexpr auto hamming1(uint8_t const a){
	std::array<uint8_t, 9> m{};

	constexpr uint8_t I = 1;

	for(size_t i = 0; i < 8; ++i){
		uint8_t const mask = I << i;
		m[i] = a ^ mask;
	}

	m[8] = a;

	if constexpr(sort){
		// std::sort(std::begin(m), std::end(m));

		using namespace hamming1_impl_;

		sort9(m);
	}

	return m;
}

auto hamming1Ranges(uint8_t const a){
	auto const values = hamming1<1>(a);

	struct Range{
		uint8_t start;
		uint8_t finish;
	};

	// grouping...
	StaticVector<Range, 8> result;

	Range range{ values[0], values[0] };

	for(auto it = std::begin(values) + 1; it != std::end(values); ++it){
		if (auto const val = *it; val == range.finish + 1){
			range.finish = val;
		}else{
			result.push_back(range);

			range = { val, val };
		}
	}

	// last range
	result.push_back(range);

	return result;
}

#endif

