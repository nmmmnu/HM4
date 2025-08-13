#ifndef MY_ENDIAN_H_
#define MY_ENDIAN_H_

#include <type_traits>

#include "mybyteswap.h"

namespace myendian_impl_{
	enum class Endian{
		LITTLE	,
		BIG	,
		UNKNOWN
	};

	constexpr auto getEndian(){
		#if	defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

			return Endian::BIG;

		#elif	defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

			return Endian::LITTLE;

		#else

			return Endian::UNKNOWN;

		#endif
	}

	static_assert(getEndian() == Endian::LITTLE || getEndian() == Endian::BIG, "I can handle only big and little endian");





	template<typename T>
	constexpr T be_byteswap(T const a){
		static_assert(std::is_unsigned_v<T>, "be_byteswap<> supports only unsigned type");

		if constexpr(getEndian() == Endian::BIG){
			return a;
		}else{
			return byteswap(a);
		}
	}

	#ifdef HAVE_UINT128_T
	// separate, because type_traits don't know about it...
	constexpr uint128_t be_byteswap(uint128_t const a){
		if constexpr(getEndian() == Endian::BIG){
			return a;
		}else{
			return byteswap(a);
		}
	}
	#endif

	constexpr uint8_t be_byteswap(uint8_t const a){
		return a;
	}

	constexpr int8_t be_byteswap(int8_t const a){
		return a;
	}

	constexpr float be_byteswap(float const a){
		if constexpr(getEndian() == Endian::BIG){
			return a;
		}else{
			return byteswap(a);
		}
	}

	constexpr double be_byteswap(double const a){
		if constexpr(getEndian() == Endian::BIG){
			return a;
		}else{
			return byteswap(a);
		}
	}





	template<typename T>
	constexpr T le_byteswap(T const a){
		static_assert(std::is_unsigned_v<T>, "be_byteswap<> supports only unsigned type");

		if constexpr(getEndian() == Endian::BIG){
			return byteswap(a);
		}else{
			return a;
		}
	}

	#ifdef HAVE_UINT128_T
	// separate, because type_traits don't know about it...
	constexpr uint128_t le_byteswap(uint128_t const a){
		if constexpr(getEndian() == Endian::BIG){
			return byteswap(a);
		}else{
			return a;
		}
	}
	#endif

	constexpr uint8_t le_byteswap(uint8_t const a){
		return a;
	}

	constexpr int8_t le_byteswap(int8_t const a){
		return a;
	}

	constexpr float le_byteswap(float const a){
		if constexpr(getEndian() == Endian::BIG){
			return byteswap(a);
		}else{
			return a;
		}
	}

	constexpr double le_byteswap(double const a){
		if constexpr(getEndian() == Endian::BIG){
			return byteswap(a);
		}else{
			return a;
		}
	}

} // namespace myendian_impl_



template <typename T>
constexpr T htobe(T const a){
	using myendian_impl_::be_byteswap;

	return be_byteswap(a);
}

template <typename T>
constexpr T betoh(T const a){
	using myendian_impl_::be_byteswap;

	return be_byteswap(a);
}



template <typename T>
constexpr T htole(T const a){
	using myendian_impl_::le_byteswap;

	return le_byteswap(a);
}

template <typename T>
constexpr T letoh(T const a){
	using myendian_impl_::le_byteswap;

	return le_byteswap(a);
}



namespace myendian_impl_{
	namespace test_{
		template<typename T>
		constexpr T htobe_test(T const a, T const be, T const le){
			auto const result = getEndian() == Endian::BIG ? be : le;
			return htobe(a) == result;
		}

		static_assert( htobe_test<uint8_t> (0x11		, 0x11			, 0x11			), "htobe<uint8_t > error" );
		static_assert( htobe_test<uint16_t>(0x1122		, 0x1122		, 0x2211		), "htobe<uint16_t> error" );
		static_assert( htobe_test<uint32_t>(0x11223344		, 0x11223344		, 0x44332211		), "htobe<uint32_t> error" );
		static_assert( htobe_test<uint64_t>(0x1122334455667788	, 0x1122334455667788	, 0x8877665544332211	), "htobe<uint64_t> error" );

		#ifdef HAVE_UINT128_T
		static_assert( htobe_test<uint128_t>(
						combine64(0x1122334455667788, 0x99aabbccddeeff55),
						combine64(0x1122334455667788, 0x99aabbccddeeff55),
						combine64(0x55ffeeddccbbaa99, 0x8877665544332211)			), "htobe<uint128_t> error" );
		#endif

	} // namespace test_
} // namespace myendian_impl_

#endif

