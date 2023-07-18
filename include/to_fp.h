#ifndef TO_FP_H_
#define TO_FP_H_

#include <string_view>
#include <limits>

// based on http://www.jbox.dk/sanos/source/lib/strtod.c.html

namespace to_fp_impl_{
	constexpr char decimal_point = '.';

	constexpr auto isspace(char c){
		return c == ' ' || c == '\t';
	};

	constexpr auto isdigit(char c){
		return c >= '0' && c <= '9';
	}

	template<typename FP>
	void add_char(FP &number, char c){
		number = number * 10 + (c - '0');
	}

	template<typename FP>
	struct ResultFP{
		FP		num;
		const char	*ep;
	};

	constexpr uint8_t exp_lookup_size = 32;

	template<typename FP>
	constexpr FP exp_lookup[exp_lookup_size]{
		/* 01 */ 1.0000000000000000000000000000000,
		/* 02 */ 0.1000000000000000000000000000000,
		/* 03 */ 0.0100000000000000000000000000000,
		/* 04 */ 0.0010000000000000000000000000000,
		/* 05 */ 0.0001000000000000000000000000000,
		/* 06 */ 0.0000100000000000000000000000000,
		/* 07 */ 0.0000010000000000000000000000000,
		/* 08 */ 0.0000001000000000000000000000000,
		/* 09 */ 0.0000000100000000000000000000000,
		/* 10 */ 0.0000000010000000000000000000000,
		/* 11 */ 0.0000000001000000000000000000000,
		/* 12 */ 0.0000000000100000000000000000000,
		/* 13 */ 0.0000000000010000000000000000000,
		/* 14 */ 0.0000000000001000000000000000000,
		/* 15 */ 0.0000000000000100000000000000000,
		/* 16 */ 0.0000000000000010000000000000000,
		/* 17 */ 0.0000000000000001000000000000000,
		/* 18 */ 0.0000000000000000100000000000000,
		/* 19 */ 0.0000000000000000010000000000000,
		/* 20 */ 0.0000000000000000001000000000000,
		/* 21 */ 0.0000000000000000000100000000000,
		/* 22 */ 0.0000000000000000000010000000000,
		/* 23 */ 0.0000000000000000000001000000000,
		/* 24 */ 0.0000000000000000000000100000000,
		/* 25 */ 0.0000000000000000000000010000000,
		/* 26 */ 0.0000000000000000000000001000000,
		/* 27 */ 0.0000000000000000000000000100000,
		/* 28 */ 0.0000000000000000000000000010000,
		/* 29 */ 0.0000000000000000000000000001000,
		/* 30 */ 0.0000000000000000000000000000100,
		/* 31 */ 0.0000000000000000000000000000010,
		/* 32 */ 0.0000000000000000000000000000001,
	};
} // namespace to_fp_impl_

// ====================

template<
	typename FP,
	uint8_t digits		= std::numeric_limits<FP>::digits10,
	uint8_t decimals	= digits
>
auto to_fp(std::string_view const str){
	using namespace to_fp_impl_;

	static_assert(
		std::is_floating_point_v<FP>				&&
		digits		<= std::numeric_limits<FP>::digits10	&&
		decimals	<= std::numeric_limits<FP>::digits10	&&
		exp_lookup_size	>= std::numeric_limits<FP>::digits10
	);

	// -----

	using Result = ResultFP<FP>;

	auto it  = std::begin(str);
	auto end = std::end(str);

	// -----

	// Skip leading whitespace
	while (it != end && isspace(*it))
		++it;

	// -----

	// Handle optional sign
	FP sign = +1;

	if (it != end){
		if (*it == '-'){
			sign = -1;

			++it;
		}else if (*it == '+'){
		//	sign = +1;

			++it;
		}
	}

	// -----

	FP number = 0.0;

	bool has_digits = false;

	// Process digits
	{
		uint8_t num_digits = 0;
		while (it != end && isdigit(*it)){
			if (num_digits >= digits)
				return Result{ 0, nullptr };

			add_char(number, *it);
			++num_digits;
			++it;
		}

		has_digits = num_digits;
	}

	// -----

	// Process decimal part
	if (it != end && *it == decimal_point){
		++it;

		uint8_t num_digits = 0;
		while (it != end && isdigit(*it)){
			if (num_digits > decimals)
				break;

			add_char(number, *it);
			++num_digits;
			++it;
		}

		has_digits |= num_digits;

		number *= exp_lookup<FP>[num_digits];
	}

	// -----

	return Result{ number * sign, has_digits ? it : nullptr };
}

template<
	typename FP,
	uint8_t digits		= std::numeric_limits<FP>::digits10,
	uint8_t decimals	= digits
>
auto to_fp_def(std::string_view const str, FP def = 0){
	auto[num, ok] = to_fp<FP, digits, decimals>(str);

	return ok ? num : def;
}

template<
	uint8_t digits		= std::numeric_limits<double>::digits10,
	uint8_t decimals	= digits
>
auto to_double_def(std::string_view const str, double def = 0){
	return to_fp_def<double, digits, decimals>(str, def);
}

template<
	uint8_t digits		= std::numeric_limits<float>::digits10,
	uint8_t decimals	= digits
>
auto to_float_def(std::string_view const str, float def = 0){
	return to_fp_def<float, digits, decimals>(str, def);
}

// ====================

template<
	typename FP,
	uint8_t digits		= std::numeric_limits<FP>::digits10,
	uint8_t decimals	= digits
>
auto to_fp_size(){
	return	1	+	// sign
		digits	+	// 12.
		1	+	// dot
		decimals	// .12345
	;
}

template<
	uint8_t digits		= std::numeric_limits<double>::digits10,
	uint8_t decimals	= digits
>
auto to_double_size(){
	return to_fp_size<double, digits, decimals>();
}

template<
	uint8_t digits		= std::numeric_limits<float>::digits10,
	uint8_t decimals	= digits
>
auto to_float_size(){
	return to_fp_size<float, digits, decimals>();
}

#endif

