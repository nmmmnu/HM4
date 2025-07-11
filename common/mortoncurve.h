#ifndef MORTON_CURVE_H_
#define MORTON_CURVE_H_

#include <cstdint>

#include "uint128_t.h"

namespace morton_curve{

	namespace morton_curve_implementation_{

		// https://stackoverflow.com/questions/18529057/produce-interleaving-bit-patterns-morton-keys-for-32-bit-64-bit-and-128bit

		constexpr uint64_t splitBits2D(uint64_t x){
			x &= 0xffff'ffff;

			x = (x | (x << 16)) & 0x0000ffff0000ffff;
			x = (x | (x <<  8)) & 0x00ff00ff00ff00ff;
			x = (x | (x <<  4)) & 0x0f0f0f0f0f0f0f0f;
			x = (x | (x <<  2)) & 0x3333333333333333;
			x = (x | (x <<  1)) & 0x5555555555555555;

			return x;
		}

		constexpr uint32_t combineBits2D(uint64_t x){
			x &=                  0x5555555555555555;

			x = (x ^ (x >>  1)) & 0x3333333333333333;
			x = (x ^ (x >>  2)) & 0x0f0f0f0f0f0f0f0f;
			x = (x ^ (x >>  4)) & 0x00ff00ff00ff00ff;
			x = (x ^ (x >>  8)) & 0x0000ffff0000ffff;
			x = (x ^ (x >> 16)) & 0x00000000ffffffff;

			return static_cast<uint32_t>(x);
		}

		#ifdef HAVE_UINT128_T

		constexpr uint128_t splitBits3D(uint128_t x){
		//	x &= 0x3f'fffff'ffff; // 42 bits max
			x &= 0xffff'ffff;

			x = (x | x << 64) & combine64(0x000003ff00000000, 0x00000000ffffffff);
			x = (x | x << 32) & combine64(0x000003ff00000000, 0xffff00000000ffff);
			x = (x | x << 16) & combine64(0x030000ff0000ff00, 0x00ff0000ff0000ff);
			x = (x | x <<  8) & combine64(0x0300f00f00f00f00, 0xf00f00f00f00f00f);
			x = (x | x <<  4) & combine64(0x030c30c30c30c30c, 0x30c30c30c30c30c3);
			x = (x | x <<  2) & combine64(0x0924924924924924, 0x9249249249249249);

			return x;
		}

		constexpr uint32_t combineBits3D(uint128_t x){
			x &=                  combine64(0x0924924924924924, 0x9249249249249249);

			x = (x ^ (x >>  2)) & combine64(0x030c30c30c30c30c, 0x30c30c30c30c30c3);
			x = (x ^ (x >>  4)) & combine64(0x0300f00f00f00f00, 0xf00f00f00f00f00f);
			x = (x ^ (x >>  8)) & combine64(0x030000ff0000ff00, 0x00ff0000ff0000ff);
			x = (x ^ (x >> 16)) & combine64(0x000003ff00000000, 0xffff00000000ffff);
			x = (x ^ (x >> 32)) & combine64(0x000003ff00000000, 0x00000000ffffffff);
			x = (x ^ (x >> 64)) & combine64(0x0000000000000000, 0x00000003ffffffff);

			return static_cast<uint32_t>(x);
		}

		constexpr uint128_t splitBits4D(uint128_t x){
			x &= 0xffff'ffff;

			x = (x | (x << 64)) & combine64(0x00000000ffc00000, 0x00000000003fffff);
			x = (x | (x << 32)) & combine64(0x00000000ffc00000, 0x003ff800000007ff);
			x = (x | (x << 16)) & combine64(0x0000f80007c0003f, 0x0000f80007c0003f);
			x = (x | (x <<  8)) & combine64(0x00c0380700c03807, 0x00c0380700c03807);
			x = (x | (x <<  4)) & combine64(0x0843084308430843, 0x0843084308430843);
			x = (x | (x <<  2)) & combine64(0x0909090909090909, 0x0909090909090909);
			x = (x | (x <<  1)) & combine64(0x1111111111111111, 0x1111111111111111);

			return x;
		}

		constexpr uint32_t combineBits4D(uint128_t x) {
			x &=                  combine64(0x11111111'11111111, 0x11111111'11111111);

			x = (x ^ (x >>  1)) & combine64(0x09090909'09090909, 0x09090909'09090909);
			x = (x ^ (x >>  2)) & combine64(0x08430843'08430843, 0x08430843'08430843);
			x = (x ^ (x >>  4)) & combine64(0x00c03807'00c03807, 0x00c03807'00c03807);
			x = (x ^ (x >>  8)) & combine64(0x0000f800'07c0003f, 0x0000f800'07c0003f);
			x = (x ^ (x >> 16)) & combine64(0x00000000'ffc00000, 0x003ff800'000007ff);
			x = (x ^ (x >> 32)) & combine64(0x00000000'ffc00000, 0x00000000'003fffff);
			x = (x ^ (x >> 64));

			return static_cast<uint32_t>(x);
		}

		#endif

	} // namespace morton_curve_implementation_



	constexpr uint64_t toMorton2D(uint32_t x, uint32_t y){
		using namespace morton_curve_implementation_;

		// xy xy xy

		auto const mx = splitBits2D(x) << 1;
		auto const my = splitBits2D(y) << 0;

		return mx | my;
	}

	constexpr auto fromMorton2D(uint64_t zzz){
		using namespace morton_curve_implementation_;

		struct Result{
			uint32_t x;
			uint32_t y;
		};

		// xy xy xy

		return Result{
			combineBits2D(zzz >> 1),
			combineBits2D(zzz >> 0)
		};
	}



	#ifdef HAVE_UINT128_T

	constexpr uint128_t toMorton3D(uint32_t x, uint32_t y, uint32_t z){
		using namespace morton_curve_implementation_;

		// xyz xyz xyz

		auto const mx = splitBits3D(x) << 2;
		auto const my = splitBits3D(y) << 1;
		auto const mz = splitBits3D(z) << 0;

		return mx | my | mz;
	}

	constexpr auto fromMorton3D(uint128_t zzz){
		using namespace morton_curve_implementation_;

		struct Result{
			uint32_t x;
			uint32_t y;
			uint32_t z;
		};

		// xyz xyz xyz

		return Result{
			combineBits3D(zzz >> 2),
			combineBits3D(zzz >> 1),
			combineBits3D(zzz >> 0)
		};
	}



	constexpr uint128_t toMorton4D(uint32_t x, uint32_t y, uint32_t z, uint32_t w){
		using namespace morton_curve_implementation_;

		// xyzw xyzw xyzw

		auto const mx = splitBits4D(x) << 3;
		auto const my = splitBits4D(y) << 2;
		auto const mz = splitBits4D(z) << 1;
		auto const mw = splitBits4D(w) << 0;

		return mx | my | mz | mw;
	}

	constexpr auto fromMorton4D(uint128_t zzz){
		using namespace morton_curve_implementation_;

		struct Result{
			uint32_t x;
			uint32_t y;
			uint32_t z;
			uint32_t w;
		};

		// xyz xyz xyz

		return Result{
			combineBits4D(zzz >> 3),
			combineBits4D(zzz >> 2),
			combineBits4D(zzz >> 1),
			combineBits4D(zzz >> 0)
		};
	}

	#endif



	uint64_t  computeBigMinFromMorton2D(uint64_t  z_current, uint64_t  z_min, uint64_t  z_max);

	#ifdef HAVE_UINT128_T

	uint128_t computeBigMinFromMorton3D(uint128_t z_current, uint128_t z_min, uint128_t z_max);
	uint128_t computeBigMinFromMorton4D(uint128_t z_current, uint128_t z_min, uint128_t z_max);

	#endif

} // namespace morton_curve

#endif




