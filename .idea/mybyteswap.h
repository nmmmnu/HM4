#ifndef MY_BYTESWAP_H_
#define MY_BYTESWAP_H_

#include <cstdint>

#include "uint128_t.h"

constexpr uint16_t byteswap(uint16_t const a){
	return __builtin_bswap16(a);
}

constexpr uint32_t byteswap(uint32_t const a){
	return __builtin_bswap32(a);
}

constexpr uint64_t byteswap(uint64_t const a){
	return __builtin_bswap64(a);
}

#ifdef HAVE_UINT128_T
constexpr uint128_t byteswap(uint128_t const а){
	auto const lo = static_cast<uint64_t>(а >>  0);
	auto const hi = static_cast<uint64_t>(а >> 64);

	return
		uint128_t{ byteswap(lo) } << 64 |
		uint128_t{ byteswap(hi) } <<  0
	;
}
#endif

// no need tests, we use intrinsics now...

#endif



/*

constexpr uint16_t byteswap(uint16_t const a){
	constexpr uint8_t b[] = {
		8 * (2 - 1)
	};

	auto const x =
		(0x00ffULL & a) << b[0] |
		(0xff00ULL & a) >> b[0]
	;

	return static_cast<uint16_t>(x);
}

constexpr uint32_t byteswap(uint32_t const a){
	constexpr uint8_t b[] = {
		8 * (2 - 1),
		8 * (4 - 1)
	};

	auto const x =
		(0x000000ffULL & a) << b[1] |
		(0x0000ff00ULL & a) << b[0] |
		(0x00ff0000ULL & a) >> b[0] |
		(0xff000000ULL & a) >> b[1]
	;

	return static_cast<uint32_t>(x);
}

constexpr uint64_t byteswap(uint64_t const a){
	constexpr uint8_t b[] = {
		8 * (2 - 1),
		8 * (4 - 1),
		8 * (6 - 1),
		8 * (8 - 1)
	};

	auto const x =
		(0x00000000000000ffULL & a) << b[3] |
		(0x000000000000ff00ULL & a) << b[2] |
		(0x0000000000ff0000ULL & a) << b[1] |
		(0x00000000ff000000ULL & a) << b[0] |
		(0x000000ff00000000ULL & a) >> b[0] |
		(0x0000ff0000000000ULL & a) >> b[1] |
		(0x00ff000000000000ULL & a) >> b[2] |
		(0xff00000000000000ULL & a) >> b[3]
	;

	return static_cast<uint64_t>(x);
}





namespace mybyteswap_impl_{
	namespace test_{
		template<typename T>
		constexpr T byteswap_test(T const a, T const b){
			return byteswap(a) == b;
		}

		static_assert( byteswap_test<uint16_t>(0x1122			, 0x2211		), "byteswap<uint16_t> error" );
		static_assert( byteswap_test<uint32_t>(0x11223344		, 0x44332211		), "byteswap<uint32_t> error" );
		static_assert( byteswap_test<uint64_t>(0x1122334455667788	, 0x8877665544332211	), "byteswap<uint64_t> error" );

		#ifdef HAVE_UINT128_T
		static_assert( byteswap_test<uint128_t>(
						combine64(0x1122334455667788, 0x99aabbccddeeff55),
						combine64(0x55ffeeddccbbaa99, 0x8877665544332211)	), "byteswap<uint128_t> error" );
		#endif
	} // namespace test_
} // namespace mybyteswap_impl_





constexpr uint128_t byteswap(uint128_t const a){
	constexpr uint8_t b[] = {
		8 * (  2 - 1),
		8 * (  4 - 1),
		8 * (  6 - 1),
		8 * (  8 - 1),
		8 * ( 10 - 1),
		8 * ( 12 - 1),
		8 * ( 14 - 1),
		8 * ( 16 - 1)
	};

	auto const x =
		(combine64(0x0000000000000000ULL, 0x00000000000000ffULL) & a) << b[7] |
		(combine64(0x0000000000000000ULL, 0x000000000000ff00ULL) & a) << b[6] |
		(combine64(0x0000000000000000ULL, 0x0000000000ff0000ULL) & a) << b[5] |
		(combine64(0x0000000000000000ULL, 0x00000000ff000000ULL) & a) << b[4] |
		(combine64(0x0000000000000000ULL, 0x000000ff00000000ULL) & a) >> b[3] |
		(combine64(0x0000000000000000ULL, 0x0000ff0000000000ULL) & a) >> b[2] |
		(combine64(0x0000000000000000ULL, 0x00ff000000000000ULL) & a) >> b[1] |
		(combine64(0x0000000000000000ULL, 0xff00000000000000ULL) & a) >> b[0] |
		(combine64(0x00000000000000ffULL, 0x0000000000000000ULL) & a) << b[0] |
		(combine64(0x000000000000ff00ULL, 0x0000000000000000ULL) & a) << b[1] |
		(combine64(0x0000000000ff0000ULL, 0x0000000000000000ULL) & a) << b[2] |
		(combine64(0x00000000ff000000ULL, 0x0000000000000000ULL) & a) << b[3] |
		(combine64(0x000000ff00000000ULL, 0x0000000000000000ULL) & a) >> b[4] |
		(combine64(0x0000ff0000000000ULL, 0x0000000000000000ULL) & a) >> b[5] |
		(combine64(0x00ff000000000000ULL, 0x0000000000000000ULL) & a) >> b[6] |
		(combine64(0xff00000000000000ULL, 0x0000000000000000ULL) & a) >> b[7]
	;

	return static_cast<uint128_t>(x);
}
*/

