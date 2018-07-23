#ifndef MY_BYTESWAP_H_
#define MY_BYTESWAP_H_

#include <cstdint>

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
	} // namespace test_
} // namespace mybyteswap_impl_

#endif

