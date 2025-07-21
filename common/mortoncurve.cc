#include "mortoncurve.h"

#include <type_traits>
#include <cstdio>

namespace morton_curve{

	namespace morton_curve_implementation_{
		constexpr auto combine3Bools(bool a, bool b, bool c){
			uint8_t result = 0;

			result |= a ? 0b0100 : 0b0000;
			result |= b ? 0b0010 : 0b0000;
			result |= c ? 0b0001 : 0b0000;

			return result;
		}

		template<uint32_t D, typename T>
		constexpr T loadBits(T const bit_pattern, T const bit_position, T const value, uint32_t const dim){
			auto splitBits_ = [](auto x){
				if constexpr(D ==  2)	return splitBits2D32(x);
				if constexpr(D ==  3)	return splitBits3D32(x);
				if constexpr(D ==  4)	return splitBits4D32(x);
				if constexpr(D ==  8)	return splitBits8D8 (x);
				if constexpr(D == 16)	return splitBits16D8(x);
			};

			auto const split  = splitBits_(bit_pattern);
			auto const insert = split << dim;

			auto const base_mask = (1u << ((bit_position / D) + 1u)) - 1u;
			auto const wipe_mask = ~(splitBits_(base_mask) << dim);

			return (value & wipe_mask) | insert;
		}

		template<uint32_t D, typename T, typename E>
		T computeBigMinFromMorton_(T xd, T z_min, T z_max){
			E const bits = sizeof(E) * 8 * D;

			E const bit_position_start = bits - 1;

			static_assert(sizeof(T) * 8 >= bits, "No room for zzz");



			auto bit_position = bit_position_start;

			T bigmin = 0u;
			T mask   = T{1} << bit_position_start;

			do{
				auto const dim = bit_position % D;
				auto const bit_mask = T{1} << (bit_position / D);

				auto const xd_bit    = xd    & mask;
				auto const z_min_bit = z_min & mask;
				auto const z_max_bit = z_max & mask;

				auto const magic = combine3Bools(xd_bit, z_min_bit, z_max_bit);

				switch(magic){
				case 0b0001:
					bigmin = loadBits<D,T>(bit_mask - 0, bit_position, z_min, dim);
					z_max  = loadBits<D,T>(bit_mask - 1, bit_position, z_max, dim);
					break;
				case 0b0010:
				case 0b0110:
					// not possible because min <= max
					// cast because limits not work with uint128_t
					return T(-1);
				case 0b0011:
					bigmin = z_min;
					return bigmin;
				case 0b0100:
					return bigmin;
				case 0b0101:
					z_min = loadBits<D,T>(bit_mask, bit_position, z_min, dim);
					break;
				default:
					break;
				}

				--bit_position;

				mask >>= 1;
			}while(mask);

			return bigmin;
		}

	} // namespace morton_curve_implementation_



	uint64_t computeBigMinFromMorton2D32(uint64_t z_current, uint64_t z_min, uint64_t z_max){
		using namespace morton_curve_implementation_;

		return computeBigMinFromMorton_<2, uint64_t, uint32_t>(z_current, z_min, z_max);
	}

	uint64_t computeBigMinFromMorton8D8(uint64_t z_current, uint64_t z_min, uint64_t z_max){
		using namespace morton_curve_implementation_;

		return computeBigMinFromMorton_<8, uint64_t, uint8_t>(z_current, z_min, z_max);
	}



	#ifdef HAVE_UINT128_T

	uint128_t computeBigMinFromMorton3D32(uint128_t z_current, uint128_t z_min, uint128_t z_max){
		using namespace morton_curve_implementation_;

		return computeBigMinFromMorton_<3, uint128_t, uint32_t>(z_current, z_min, z_max);
	}

	uint128_t computeBigMinFromMorton4D32(uint128_t z_current, uint128_t z_min, uint128_t z_max){
		using namespace morton_curve_implementation_;

		return computeBigMinFromMorton_<4, uint128_t, uint32_t>(z_current, z_min, z_max);
	}

	uint128_t computeBigMinFromMorton16D8(uint128_t z_current, uint128_t z_min, uint128_t z_max){
		using namespace morton_curve_implementation_;

		return computeBigMinFromMorton_<16, uint128_t, uint8_t>(z_current, z_min, z_max);
	}

	#endif

} // namespace morton_curve








