#include "mortoncurve.h"

#include <cassert>
#include <cstdio>

int main(){
	using namespace morton_curve::morton_curve_implementation_;

	if constexpr(1){
		for(uint32_t i = 0; i < 0xFFFFFFFF; ++i){
			{
				auto const zzz = splitBits2D32(i);
				auto const b = combineBits2D32(zzz);
				assert(i == b);
			}

			#ifdef HAVE_UINT128_T

			{
				auto const zzz = splitBits3D32(i);
				auto const b = combineBits3D32(zzz);
				assert(i == b);
			}

			{
				auto const zzz = splitBits4D32(i);
				auto const b = combineBits4D32(zzz);
				assert(i == b);
			}

			#endif

			if (i % 0xFFFFFF == 0)
				printf("%10u of %10u\n", i, 0xFFFFFFFF);
		}
	}

	if constexpr(1){
		for(uint32_t i = 0; i < 0xFF; ++i){
			#ifdef HAVE_UINT128_T

			{
				auto const zzz = splitBits16D8(i);
				auto const b = combineBits16D8(zzz);
				assert(i == b);
			}

			#endif
		}
	}
}
