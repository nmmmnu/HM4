#include <cstdio>
#include <cstdint>

namespace myendian_impl_{

	template<typename UINT>
	UINT byteswap(UINT const a);

	template<>
	constexpr uint16_t byteswap(uint16_t const a){
		constexpr uint8_t b[] = {
			8 * (2 - 1)
		};

		auto const x =
			(0x00ffULL & a) << b[0]	|
			(0xff00ULL & a) >> b[0]
		;

		return static_cast<uint16_t>(x);
	}

	template<>
	constexpr uint32_t byteswap(uint32_t const a){
		constexpr uint8_t b[] = {
			8 * (2 - 1),
			8 * (4 - 1)
		};

		auto const x =
			(0x000000ffULL & a) << b[1]	|
			(0x0000ff00ULL & a) << b[0]	|
			(0x00ff0000ULL & a) >> b[0]	|
			(0xff000000ULL & a) >> b[1]
		;

		return static_cast<uint32_t>(x);
	}

	template<>
	constexpr uint64_t byteswap(uint64_t const a){
		constexpr uint8_t b[] = {
			8 * (2 - 1),
			8 * (4 - 1),
			8 * (6 - 1),
			8 * (8 - 1)
		};

		auto const x =
			(0x00000000000000ffULL & a) << b[3]	|
			(0x000000000000ff00ULL & a) << b[2]	|
			(0x0000000000ff0000ULL & a) << b[1]	|
			(0x00000000ff000000ULL & a) << b[0]	|
			(0x000000ff00000000ULL & a) >> b[0]	|
			(0x0000ff0000000000ULL & a) >> b[1]	|
			(0x00ff000000000000ULL & a) >> b[2]	|
			(0xff00000000000000ULL & a) >> b[3]
		;

		return static_cast<uint64_t>(x);
	}

	// ==============================

	class isBE{
		constexpr static uint32_t u4 = 1;
		constexpr static uint8_t  u1  = (const uint8_t &) u4;
	public:
		constexpr static bool value = u1 == 0;
	};

	// ==============================

	template<bool b>
	struct be_tag{};

	// ==============================

	template<typename UINT>
	constexpr UINT htobe(UINT const a, be_tag<true>){
		return a;
	}

	template<typename UINT>
	constexpr UINT htobe(UINT const a, be_tag<false>){
		return byteswap(a);
	}

} // namespace myendian_impl_

constexpr inline uint16_t htobe16(uint16_t const a){
	using namespace  myendian_impl_;
	return htobe(a, be_tag<isBE::value>{});
}

constexpr inline uint32_t htobe32(uint32_t const a){
	using namespace  myendian_impl_;
	return htobe(a, be_tag<isBE::value>{});
}

constexpr inline uint64_t htobe64(uint64_t const a){
	using namespace  myendian_impl_;
	return htobe(a, be_tag<isBE::value>{});
}


int main(){
	printf("%16x %16x\n",	0x1122,			htobe16(0x1122)			);
	printf("%16x %16x\n",	0x11223344,		htobe32(0x11223344)		);
	printf("%16lx %16lx\n",	0x1122334455667788,	htobe64(0x1122334455667788)	);
}

