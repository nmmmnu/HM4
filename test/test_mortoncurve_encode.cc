#include "mortoncurve.h"

#include <cassert>
#include <cstdio>

int main(){
	using namespace morton_curve::morton_curve_implementation_;

	for(uint32_t i = 0; i < 0xFFFFFFFF; ++i){
		{
			auto const zzz = splitBits2D(i);
			auto const b = combineBits2D(zzz);
			assert(i == b);
		}

		{
			auto const zzz = splitBits3D(i);
			auto const b = combineBits3D(zzz);
			assert(i == b);
		}

		{
			auto const zzz = splitBits4D(i);
			auto const b = combineBits4D(zzz);
			assert(i == b);
		}

		if (i % 0xFFFFFF == 0)
			printf("%10u of %10u\n", i, 0xFFFFFFFF);
	}
}
