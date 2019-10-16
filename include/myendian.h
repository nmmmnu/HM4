#ifndef MY_ENDIAN_H_
#define MY_ENDIAN_H_

#include <type_traits>

#include "mybyteswap.h"

namespace myendian_impl_{
	class is_be{
	private:
		constexpr static uint32_t host = 0x00112233;
		constexpr static uint16_t mem  = (const uint16_t &) host;

	private:
		static_assert(mem == 0x0011 || mem == 0x2233, "I can handle only big and little endian");

	public:
		constexpr static bool value = mem == 0x0011;

		using type = std::bool_constant<value>;

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

