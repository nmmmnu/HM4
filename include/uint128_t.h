#ifndef UINT128_T_H_
#define UINT128_T_H_

	#ifdef __SIZEOF_INT128__

		#define HAVE_UINT128_T

		#include <cstdint>

		using uint128_t = __uint128_t;

		constexpr uint128_t combine64(uint64_t hi, uint64_t lo){
			return (uint128_t{ hi } << 64) | lo;
		};

	#endif

#endif

