#include "myalign.h"

#include "mytest.h"

MyTest mytest;

// ==================================

template<uint16_t ALIGN>
void fn_alc();

// ==================================

int main(){
	fn_alc<2>();
	fn_alc<4>();
	fn_alc<8>();
	fn_alc<16>();
	fn_alc<32>();
	fn_alc<64>();

	return mytest.end();
}

// ==================================


template<class ALC>
bool fn_alc_test(const ALC &alc, size_t const size, size_t const expect1){
	size_t const expect2 = expect1 - size;

	return
		alc.calc(size)		== expect1	&&
		alc.padding(size)	== expect2
	;
}

template<uint16_t ALIGN>
void fn_alc(){
	constexpr MyAlign alc(ALIGN);

	for(size_t i = 0; i < 10 * 1024; ++i){
		size_t a = i;

		while(a % ALIGN != 0)
			++a;

		//printf("%5zu -> %5zu\n", i, a);

		bool const b = fn_alc_test(alc,  i,  a);

		if (!b)
			mytest("", false);
	}
}

