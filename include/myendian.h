#ifndef MY_ENDIAN_H_
#define MY_ENDIAN_H_

#include <type_traits>

#include "mybyteswap.h"

namespace myendian_impl_{
	class is_be{
	private:
		constexpr static uint32_t u4 = 0x11223344;
		constexpr static uint8_t  u1 = (const uint8_t  &) u4;

	private:
		constexpr static uint32_t t4 = (const uint32_t &) u4;
		static_assert(t4 == 0x11223344 || t4 == 0x44332211, "I can handle only big and little endian");

	public:
		constexpr static bool value = u1 == 0x11;

		using type = std::integral_constant<bool, value>;

	//	static_assert(value == false, "");
	};



	template<typename T>
	constexpr T be_byteswap(T const a, std::true_type){
		return a;
	}

	template<typename T>
	constexpr T be_byteswap(T const a, std::false_type){
		return byteswap(a);
	}

	template<typename T>
	constexpr T be_byteswap(T const a){
		static_assert(std::is_unsigned<T>::value, "be_byteswap<> supports only unsigned type");

		return be_byteswap(a, is_be::type{});
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



namespace myendian_test_{
	template<typename T>
	constexpr T htobe_test(T const a, T const be, T const le){
		using myendian_impl_::is_be;

		auto const result = is_be::value ? be : le;
		return htobe(a) == result;
	}

	static_assert( htobe_test<uint16_t>(0x1122		, 0x1122		, 0x2211		), "htobe<uint16_t> error" );
	static_assert( htobe_test<uint32_t>(0x11223344		, 0x11223344		, 0x44332211		), "htobe<uint32_t> error" );
	static_assert( htobe_test<uint64_t>(0x1122334455667788	, 0x1122334455667788	, 0x8877665544332211	), "htobe<uint64_t> error" );
} // namespace myendian_test_

#endif

