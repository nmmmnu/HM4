#ifndef MY_ENDIAN_FP_H_
#define MY_ENDIAN_FP_H_

#include "myendian.h"

namespace myendian_impl_{
	template<typename To, typename From>
	auto fp_cast(From value){
		const auto *x = reinterpret_cast<To *>(&value);
		return *x;
	}

	template<typename FP, typename T>
	FP swapFP(FP value){
		static_assert(sizeof(FP) == sizeof(T));

		auto const x = fp_cast<T>(value);

		return fp_cast<FP>(be_byteswap(x));
	}

	double swapDouble(double value){
		return swapFP<double, uint64_t>(value);
	}

	float swapFloat(float value){
		return swapFP<float, uint32_t>(value);
	}

} // namespace myendian_impl_


inline float betoh(float value) {
	return myendian_impl_::swapFloat(value);
}

inline double betoh(double value) {
	return myendian_impl_::swapDouble(value);
}



inline float htobe(float value) {
	return myendian_impl_::swapFloat(value);
}

inline double htobe(double value) {
	return myendian_impl_::swapDouble(value);
}

#endif

