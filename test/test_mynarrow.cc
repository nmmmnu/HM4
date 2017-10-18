#include "mynarrow.h"

#include <cassert>
#include <cstdint>

template<typename T1, typename T2>
void test(){
	constexpr T2 num = 1;

	static_assert(narrow<T1, T2>(num) == num, "");
}

template<typename T1, typename T2>
void test_max(){
	constexpr T2 num = std::numeric_limits<T2>::max();

	static_assert(narrow<T1, T2>(num) == num, "");
}

template<typename T1, typename T2>
void test_over(){
	try{
		constexpr T2 num = std::numeric_limits<T2>::max();

		narrow<T1>(num);
		// must throw...
		assert(false);
	}catch(const std::exception &e){
	}
}

template<typename T1>
void test_negative(){
	try{
		constexpr int8_t num = -1;

		narrow<T1>(num);
		// must throw...
		assert(false);
	}catch(const std::exception &e){
	}
}


int main(){
	test<int16_t,	int8_t		>();

	test		<int16_t,	int8_t		>();
	test		<int16_t,	int16_t		>();
	test		<int16_t,	int32_t		>();
	test		<int16_t,	int64_t		>();
	test		<int16_t,	float		>();
	test		<int16_t,	double		>();

	test		<int16_t,	uint8_t		>();
	test		<int16_t,	uint16_t	>();
	test		<int16_t,	uint32_t	>();
	test		<int16_t,	uint64_t	>();

	test_max	<uint16_t,	uint8_t		>();
	test_max	<uint32_t,	uint16_t	>();
	test_max	<uint64_t,	uint32_t	>();

	test_max	<uint16_t,	int8_t		>();
	test_max	<uint32_t,	int16_t		>();
	test_max	<uint64_t,	int32_t		>();

	test_over	<int8_t,	int16_t		>();
	test_over	<int16_t,	int32_t		>();
	test_over	<int32_t,	int64_t		>();

	test_negative	<uint8_t	>();
	test_negative	<uint16_t	>();
	test_negative	<uint32_t	>();
}


