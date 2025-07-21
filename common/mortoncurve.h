#ifndef MORTON_CURVE_H_
#define MORTON_CURVE_H_

#include <cstdint>
#include <array>

#include "uint128_t.h"

namespace morton_curve{

	namespace morton_curve_implementation_{

		// https://stackoverflow.com/questions/18529057/produce-interleaving-bit-patterns-morton-keys-for-32-bit-64-bit-and-128bit

		constexpr uint64_t splitBits2D32(uint64_t x){
			x &= 0xffff'ffff;

			x = (x | (x << 16)) & 0x0000ffff0000ffff;
			x = (x | (x <<  8)) & 0x00ff00ff00ff00ff;
			x = (x | (x <<  4)) & 0x0f0f0f0f0f0f0f0f;
			x = (x | (x <<  2)) & 0x3333333333333333;
			x = (x | (x <<  1)) & 0x5555555555555555;

			return x;
		}

		constexpr uint32_t combineBits2D32(uint64_t x){
			x &=                  0x5555555555555555;

			x = (x ^ (x >>  1)) & 0x3333333333333333;
			x = (x ^ (x >>  2)) & 0x0f0f0f0f0f0f0f0f;
			x = (x ^ (x >>  4)) & 0x00ff00ff00ff00ff;
			x = (x ^ (x >>  8)) & 0x0000ffff0000ffff;
			x = (x ^ (x >> 16)) & 0x00000000ffffffff;

			return static_cast<uint32_t>(x);
		}

		constexpr uint64_t splitBits8D8(uint64_t x){
			x &= 0xff;
			x = (x | (x << 32)) & 0x00000e00000001f;
			x = (x | (x << 16)) & 0x080006000180007;
			x = (x | (x <<  8)) & 0x080402010080403;
			x = (x | (x <<  4)) & 0x080402100804021;
			x = (x | (x <<  2)) & 0x081008100810081;
			x = (x | (x <<  1)) & 0x101010101010101;

			return x;
		}

		constexpr uint8_t combineBits8D8(uint64_t x){
			uint64_t const I = 1;

			uint64_t const result =
				(x & (I << 8 * 0)) >> (8 * 0 - 0) |
				(x & (I << 8 * 1)) >> (8 * 1 - 1) |
				(x & (I << 8 * 2)) >> (8 * 2 - 2) |
				(x & (I << 8 * 3)) >> (8 * 3 - 3) |
				(x & (I << 8 * 4)) >> (8 * 4 - 4) |
				(x & (I << 8 * 5)) >> (8 * 5 - 5) |
				(x & (I << 8 * 6)) >> (8 * 6 - 6) |
				(x & (I << 8 * 7)) >> (8 * 7 - 7)
			;

			return static_cast<uint8_t>(result);
		}

		#ifdef HAVE_UINT128_T

		constexpr uint128_t splitBits3D32(uint128_t x){
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

		constexpr uint32_t combineBits3D32(uint128_t x){
			x &=                  combine64(0x0924924924924924, 0x9249249249249249);

			x = (x ^ (x >>  2)) & combine64(0x030c30c30c30c30c, 0x30c30c30c30c30c3);
			x = (x ^ (x >>  4)) & combine64(0x0300f00f00f00f00, 0xf00f00f00f00f00f);
			x = (x ^ (x >>  8)) & combine64(0x030000ff0000ff00, 0x00ff0000ff0000ff);
			x = (x ^ (x >> 16)) & combine64(0x000003ff00000000, 0xffff00000000ffff);
			x = (x ^ (x >> 32)) & combine64(0x000003ff00000000, 0x00000000ffffffff);
			x = (x ^ (x >> 64)) & combine64(0x0000000000000000, 0x00000003ffffffff);

			return static_cast<uint32_t>(x);
		}

		constexpr uint128_t splitBits4D32(uint128_t x){
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

		constexpr uint32_t combineBits4D32(uint128_t x){
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

		constexpr uint128_t splitBits16D8(uint128_t x){
			x &= 0xff;
			x = (x | (x << 64)) & combine64(0x00000000000000e0, 0x000000000000001f);
			x = (x | (x << 32)) & combine64(0x0000008000000060, 0x0000001800000007);
			x = (x | (x << 16)) & combine64(0x0000008000400020, 0x0010000800040003);
			x = (x | (x <<  8)) & combine64(0x0000800040002000, 0x1000080004000201);
			x = (x | (x <<  4)) & combine64(0x0000800040002001, 0x0000800040002001);
			x = (x | (x <<  2)) & combine64(0x0000800100008001, 0x0000800100008001);
			x = (x | (x <<  1)) & combine64(0x0001000100010001, 0x0001000100010001);

			return x;
		}

		constexpr uint8_t combineBits16D8(uint128_t x){
			uint128_t const I = 1;

			uint128_t const result =
				(x & (I << 16 * 0)) >> (16 * 0 - 0) |
				(x & (I << 16 * 1)) >> (16 * 1 - 1) |
				(x & (I << 16 * 2)) >> (16 * 2 - 2) |
				(x & (I << 16 * 3)) >> (16 * 3 - 3) |
				(x & (I << 16 * 4)) >> (16 * 4 - 4) |
				(x & (I << 16 * 5)) >> (16 * 5 - 5) |
				(x & (I << 16 * 6)) >> (16 * 6 - 6) |
				(x & (I << 16 * 7)) >> (16 * 7 - 7)
			;

			return static_cast<uint8_t>(result);
		}

		#endif

	} // namespace morton_curve_implementation_



	constexpr uint64_t toMorton2D32(std::array<uint32_t, 2> v){
		using namespace morton_curve_implementation_;

		// xy xy

		return
			splitBits2D32(v[ 0]) <<  0 |
			splitBits2D32(v[ 1]) <<  1
		;
	}

	constexpr auto fromMorton2D32(uint64_t zzz){
		using namespace morton_curve_implementation_;

		// xy xy

		return std::array<uint32_t, 2>{
			combineBits2D32(zzz >> 0),
			combineBits2D32(zzz >> 1)
		};
	}

	constexpr uint64_t toMorton8D8(std::array<uint8_t, 8> const &v){
		using namespace morton_curve_implementation_;

		// 01234567 01234567 01234567 01234567

		return
			splitBits8D8(v[ 0]) <<  0 |
			splitBits8D8(v[ 1]) <<  1 |
			splitBits8D8(v[ 2]) <<  2 |
			splitBits8D8(v[ 3]) <<  3 |

			splitBits8D8(v[ 4]) <<  4 |
			splitBits8D8(v[ 5]) <<  5 |
			splitBits8D8(v[ 6]) <<  6 |
			splitBits8D8(v[ 7]) <<  7
		;
	}

	constexpr auto fromMorton8D8(uint64_t zzz){
		using namespace morton_curve_implementation_;

		// 01234567 01234567 01234567 01234567

		return std::array<uint8_t, 8>{
			combineBits8D8(zzz >>  0),
			combineBits8D8(zzz >>  1),
			combineBits8D8(zzz >>  2),
			combineBits8D8(zzz >>  3),

			combineBits8D8(zzz >>  4),
			combineBits8D8(zzz >>  5),
			combineBits8D8(zzz >>  6),
			combineBits8D8(zzz >>  7)
		};
	}



	#ifdef HAVE_UINT128_T

	constexpr uint128_t toMorton3D32(std::array<uint32_t, 3> v){
		using namespace morton_curve_implementation_;

		// xyz xyz xyz

		return
			splitBits3D32(v[ 0]) <<  0 |
			splitBits3D32(v[ 1]) <<  1 |
			splitBits3D32(v[ 2]) <<  2
		;
	}

	constexpr auto fromMorton3D32(uint128_t zzz){
		using namespace morton_curve_implementation_;

		// xyz xyz xyz

		return std::array<uint32_t, 3>{
			combineBits3D32(zzz >> 0),
			combineBits3D32(zzz >> 1),
			combineBits3D32(zzz >> 2)
		};
	}



	constexpr uint128_t toMorton4D32(std::array<uint32_t, 4> v){
		using namespace morton_curve_implementation_;

		// xyzw xyzw xyzw xyzw

		return
			splitBits4D32(v[ 0]) <<  0 |
			splitBits4D32(v[ 1]) <<  1 |
			splitBits4D32(v[ 2]) <<  2 |
			splitBits4D32(v[ 3]) <<  3
		;

	}

	constexpr auto fromMorton4D32(uint128_t zzz){
		using namespace morton_curve_implementation_;

		// xyzw xyzw xyzw xyzw

		return std::array<uint32_t, 4>{
			combineBits4D32(zzz >> 0),
			combineBits4D32(zzz >> 1),
			combineBits4D32(zzz >> 2),
			combineBits4D32(zzz >> 3)
		};
	}

	constexpr uint128_t toMorton16D8(std::array<uint8_t, 16> const &v){
		using namespace morton_curve_implementation_;

		// 01234567 01234567 01234567 01234567
		// 01234567 01234567 01234567 01234567

		return
			splitBits16D8(v[ 0]) <<  0 |
			splitBits16D8(v[ 1]) <<  1 |
			splitBits16D8(v[ 2]) <<  2 |
			splitBits16D8(v[ 3]) <<  3 |
			splitBits16D8(v[ 4]) <<  4 |
			splitBits16D8(v[ 5]) <<  5 |
			splitBits16D8(v[ 6]) <<  6 |
			splitBits16D8(v[ 7]) <<  7 |
			splitBits16D8(v[ 8]) <<  8 |
			splitBits16D8(v[ 9]) <<  9 |
			splitBits16D8(v[10]) << 10 |
			splitBits16D8(v[11]) << 11 |
			splitBits16D8(v[12]) << 12 |
			splitBits16D8(v[13]) << 13 |
			splitBits16D8(v[14]) << 14 |
			splitBits16D8(v[15]) << 15
		;
	}

	constexpr auto fromMorton16D8(uint128_t zzz){
		using namespace morton_curve_implementation_;

		// 01234567 01234567 01234567 01234567
		// 01234567 01234567 01234567 01234567

		return std::array<uint8_t, 16>{
			combineBits16D8(zzz >>  0),
			combineBits16D8(zzz >>  1),
			combineBits16D8(zzz >>  2),
			combineBits16D8(zzz >>  3),

			combineBits16D8(zzz >>  4),
			combineBits16D8(zzz >>  5),
			combineBits16D8(zzz >>  6),
			combineBits16D8(zzz >>  7),

			combineBits16D8(zzz >>  8),
			combineBits16D8(zzz >>  9),
			combineBits16D8(zzz >> 10),
			combineBits16D8(zzz >> 11),

			combineBits16D8(zzz >> 12),
			combineBits16D8(zzz >> 13),
			combineBits16D8(zzz >> 14),
			combineBits16D8(zzz >> 15)
		};
	}

	#endif



	uint64_t  computeBigMinFromMorton2D32(uint64_t  z_current, uint64_t  z_min, uint64_t  z_max);
	uint64_t  computeBigMinFromMorton8D8 (uint64_t  z_current, uint64_t  z_min, uint64_t  z_max);

	#ifdef HAVE_UINT128_T

	uint128_t computeBigMinFromMorton3D32(uint128_t z_current, uint128_t z_min, uint128_t z_max);
	uint128_t computeBigMinFromMorton4D32(uint128_t z_current, uint128_t z_min, uint128_t z_max);

	uint128_t computeBigMinFromMorton16D8(uint128_t z_current, uint128_t z_min, uint128_t z_max);

	#endif

} // namespace morton_curve

#endif




