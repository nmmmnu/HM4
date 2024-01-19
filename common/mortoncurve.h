#ifndef MORTON_CURVE_H_
#define MORTON_CURVE_H_

#include <cstdint>

namespace morton_curve{

	namespace morton_curve_implementation_{
		constexpr uint64_t splitBits(uint64_t value){
			value &= 0xffff'ffff;

			value = (value ^ (value << 16)) & 0x0000'ffff'0000'ffff;
			value = (value ^ (value <<  8)) & 0x00ff'00ff'00ff'00ff;
			value = (value ^ (value <<  4)) & 0x0f0f'0f0f'0f0f'0f0f;
			value = (value ^ (value <<  2)) & 0x3333'3333'3333'3333;
			value = (value ^ (value <<  1)) & 0x5555'5555'5555'5555;

			return value;
		}

		constexpr uint64_t combineBits(uint64_t value){
			value &= 0x5555'5555'5555'5555;

			value = (value ^ (value >>  1)) & 0x3333'3333'3333'3333;
			value = (value ^ (value >>  2)) & 0x0f0f'0f0f'0f0f'0f0f;
			value = (value ^ (value >>  4)) & 0x00ff'00ff'00ff'00ff;
			value = (value ^ (value >>  8)) & 0x0000'ffff'0000'ffff;
			value = (value ^ (value >> 16)) & 0x0000'0000'ffff'ffff;

			return value;
		}
	} // namespace morton_curve_implementation_

	constexpr uint64_t toMorton2D(uint32_t x, uint32_t y){
		using namespace morton_curve_implementation_;

		return  (splitBits(x) << 1) |
			(splitBits(y) << 0);
	}

	constexpr auto fromMorton2D(uint64_t z){
		using namespace morton_curve_implementation_;

		auto combineBits32 = [](uint64_t value){
			return static_cast<uint32_t>(
				combineBits(value)
			);
		};

		struct Result{
			uint32_t x;
			uint32_t y;
		};

		return Result{
			combineBits32(z >> 1),
			combineBits32(z >> 0)
		};
	}

	uint64_t computeBigMinFromMorton2D(uint64_t z_current, uint64_t z_min, uint64_t z_max);

} // namespace morton_curve

#endif


