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

	constexpr static auto check__(){
		#if	defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

			return Endian::BIG;

		#elif	defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

			return Endian::LITTLE;

		#else

			return Endian::UNKNOWN;

		#endif
	}

	static_assert(check__() == Endian::LITTLE || check__() == Endian::BIG, "I can handle only big and little endian");

	namespace is_be{
		constexpr static bool value = check__() == Endian::BIG;
	};

	template<typename T>
	constexpr T be_byteswap(T const a){
		static_assert(std::is_unsigned<T>::value, "be_byteswap<> supports only unsigned type");

		if constexpr(is_be::value){
			return a;
		}else{
			return byteswap(a);
		}
	}

	constexpr uint8_t be_byteswap(uint8_t const a){
		return a;
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



namespace myendian_impl_{
	namespace test_{
		template<typename T>
		constexpr T htobe_test(T const a, T const be, T const le){
			auto const result = is_be::value ? be : le;
			return htobe(a) == result;
		}

		static_assert( htobe_test<uint8_t> (0x11		, 0x11			, 0x11			), "htobe<uint8_t > error" );
		static_assert( htobe_test<uint16_t>(0x1122		, 0x1122		, 0x2211		), "htobe<uint16_t> error" );
		static_assert( htobe_test<uint32_t>(0x11223344		, 0x11223344		, 0x44332211		), "htobe<uint32_t> error" );
		static_assert( htobe_test<uint64_t>(0x1122334455667788	, 0x1122334455667788	, 0x8877665544332211	), "htobe<uint64_t> error" );
	} // namespace test_
} // namespace myendian_impl_

#endif

