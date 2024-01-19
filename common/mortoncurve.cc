#include "mortoncurve.h"

#include <limits>

namespace morton_curve{
	namespace morton_curve_implementation_{
		namespace{
			constexpr uint64_t loadBits(uint64_t bit_pattern, uint64_t const bit_position, uint64_t const value, uint64_t const dim){
				// dim = 0 for x; dim = 1 for y

				uint64_t wipe_mask = ~(splitBits(0xffff'ffff >> (32u - (bit_position / 2u + 1u))) << dim);
				bit_pattern = splitBits(bit_pattern) << dim;
				return (value & wipe_mask) | bit_pattern;
			}

			constexpr auto combine3Bools(bool a, bool b, bool c){
				uint8_t result = 0;

				result |= a ? 0b0100 : 0b0000;
				result |= b ? 0b0010 : 0b0000;
				result |= c ? 0b0001 : 0b0000;

				return result;
			}
		} // anonymous namespace
	} // namespace morton_curve_implementation_



	uint64_t computeBigMinFromMorton2D(uint64_t const xd, uint64_t z_min, uint64_t z_max){
		using namespace morton_curve_implementation_;

		uint64_t bigmin = 0u;
		uint64_t mask = 0x8000'0000'0000'0000;
		uint32_t bit_position = 63u;

		do{
			auto const dim = bit_position % 2u;
			auto const bit_mask = 0x1 << (bit_position / 2u);

			auto const xd_bit    = xd    & mask;
			auto const z_min_bit = z_min & mask;
			auto const z_max_bit = z_max & mask;

			auto const magic = combine3Bools(xd_bit, z_min_bit, z_max_bit);

			switch(magic){
			case 0b0001:
				bigmin = loadBits(bit_mask - 0, bit_position, z_min, dim);
				z_max  = loadBits(bit_mask - 1, bit_position, z_max, dim);
				break;
			case 0b0010:
			case 0b0110:
				// not possible because min <= max
				return std::numeric_limits<uint64_t>::max();
			case 0b0011:
				bigmin = z_min;
				return bigmin;
			case 0b0100:
				return bigmin;
			case 0b0101:
				z_min = loadBits(bit_mask, bit_position, z_min, dim);
				break;
			default:
				break;
			}

			--bit_position;

			mask >>= 1;
		}while(mask);

		return bigmin;
	}

} // namespace morton_curve

