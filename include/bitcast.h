#ifndef MY_BIT_CAST_H_
#define MY_BIT_CAST_H_

template<typename To, typename From>
constexpr To bit_cast(From value){
	#if 0
		const auto *x = reinterpret_cast<To *>(&value);
		return *x;
	#else
		return __builtin_bit_cast(To, value);
	#endif
}

#endif

