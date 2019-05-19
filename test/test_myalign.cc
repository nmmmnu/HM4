#include "myalign.h"

#include "mytest.h"

MyTest mytest;


namespace {

	inline bool fn_alc_test(size_t const size, uint16_t align, size_t const expect1){
		return my_align::calc(size, align)	== expect1;
	}


	void fn_alc(uint16_t align){
		for(size_t i = 0; i < 10 * 1024; ++i){
			size_t a = i;

			while(a % align != 0)
				++a;

			bool const b = fn_alc_test(i, align, a);

			if (!b)
				mytest("", false);
		}
	}

} // namespace


int main(){
	fn_alc( 2);
	fn_alc( 4);
	fn_alc( 8);
	fn_alc(16);
	fn_alc(32);
	fn_alc(64);

	return mytest.end();
}

