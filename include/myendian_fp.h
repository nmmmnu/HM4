#ifndef MY_ENDIAN_FP_H_
#define MY_ENDIAN_FP_H_

#include "myendian.h"

namespace myendian_impl_{
	template<typename To, typename From>
	constexpr To bit_cast(From value){
		#if 0
			const auto *x = reinterpret_cast<To *>(&value);
			return *x;
		#else
			return __builtin_bit_cast(To, value);
		#endif
	}

	template<typename FP, typename T>
	constexpr FP swapFP(FP value){
		static_assert(sizeof(FP) == sizeof(T));

		auto const x = bit_cast<T>(value);

		return bit_cast<FP>(be_byteswap(x));
	}

	constexpr double swapDouble(double value){
		return swapFP<double, uint64_t>(value);
	}

	constexpr float swapFloat(float value){
		return swapFP<float, uint32_t>(value);
	}

} // namespace myendian_impl_



constexpr float betoh(float value) {
	return myendian_impl_::swapFloat(value);
}

constexpr double betoh(double value) {
	return myendian_impl_::swapDouble(value);
}



constexpr float htobe(float value) {
	return myendian_impl_::swapFloat(value);
}

constexpr double htobe(double value) {
	return myendian_impl_::swapDouble(value);
}



namespace myendian_impl_{
	namespace test_{
		static_assert( betoh(0.0f  	 ) == 0.00f, "betoh 0 error"		);
		static_assert( betoh(0.0f  	 ) == 0.00f, "htobe 0 error"		);
		static_assert( betoh(htobe(5.25f)) == 5.25f, "htobe/betoh error"	);

		static_assert( betoh(0.0  	 ) == 0.00 , "betoh 0 error"		);
		static_assert( betoh(0.0  	 ) == 0.00 , "htobe 0 error"		);
		static_assert( betoh(htobe(5.25 )) == 5.25 , "htobe/betoh error"	);
	} // namespace test_
} // namespace myendian_impl_


#endif

