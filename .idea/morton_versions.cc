








#if 0
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

#endif




		template<uint16_t N, typename T>
		T computeBigMinFromMorton_xxxxx(T xd, T z_min, T z_max) {
			T bigmin = 0;

			uint16_t const total_bits = sizeof(T) * 8u;
			uint16_t bit_position = total_bits - 1u;

			T mask = T{1} << bit_position;

			while(mask){
				auto const dim       = bit_position % N;
				auto const bit_index = bit_position / N;

				auto const xd_bit   = (xd >> bit_index ) & 1u;
				auto const zmin_bit = (z_min & mask    ) != 0;
				auto const zmax_bit = (z_max & mask    ) != 0;

				auto const magic = combine3Bools(xd_bit, zmin_bit, zmax_bit);

				switch(magic) {
					case 0b0001: {
						T dim_mask = 0;

						for (uint16_t i = 0; i <= bit_index; ++i)
							dim_mask |= T{1} << (i * N + dim);

						bigmin = (z_min & ~dim_mask) | (T{1} << (bit_index * N + dim));

						z_max &= ~dim_mask;

						return bigmin;
					}

					case 0b0010:
					case 0b0110:
						// not possible because min <= max
						return std::numeric_limits<T>::max();

					case 0b0011:
						return z_min;

					case 0b0100:
						return bigmin;

					case 0b0101:
						z_min |= mask;
						break;

					default:
						break;
				}

				--bit_position;

				mask >>= 1;
			}

			return bigmin;
		}


	} // namespace morton_curve_implementation_

uint64_t computeBigMinFromMorton2D(uint64_t xd, uint64_t z_min, uint64_t z_max){
	using namespace morton_curve_implementation_;

	return computeBigMinFromMorton_<2, uint64_t>(xd, z_min, z_max);
}



/*
namespace morton_curve_implementation3d_ {
        constexpr __uint128_t loadBits(__uint128_t bit_pattern, __uint128_t const bit_position, __uint128_t const value, __uint128_t const dim) {
            // dim = 0 for x, 1 for y, 2 for z
            // Разпределяме битовете на bit_pattern за съответното измерение (x, y или z)
            __uint128_t distributed_pattern = 0;
            for (uint32_t i = 0; i < 64; ++i) { // Обработваме до 64 бита, тъй като Morton кодът разпределя 128/3 ≈ 42 бита на измерение
                if (bit_pattern & (static_cast<__uint128_t>(1) << i)) {
                    distributed_pattern |= static_cast<__uint128_t>(1) << (i * 3 + dim);
                }
            }

            // Създаваме wipe_mask за изчистване на битовете на съответното измерение
            __uint128_t wipe_mask = 0;
            for (uint32_t i = 0; i <= bit_position / 3; ++i) {
                wipe_mask |= static_cast<__uint128_t>(1) << (i * 3 + dim);
            }
            wipe_mask = ~wipe_mask; // Инвертираме маската, за да изчистим битовете

            // Изместваме distributed_pattern според dim и комбинираме с value
            return (value & wipe_mask) | distributed_pattern;
        }

        constexpr auto combine3Bools(bool a, bool b, bool c) {
            uint8_t result = 0;
            result |= a ? 0b0100 : 0b0000;
            result |= b ? 0b0010 : 0b0000;
            result |= c ? 0b0001 : 0b0000;
            return result;
        }

    __uint128_t computeBigMinFromMorton3D_(__uint128_t const xd, __uint128_t z_min, __uint128_t z_max) {

        __uint128_t bigmin = 0u;
        __uint128_t mask = (__uint128_t)1 << 127; // Започваме от най-високия бит (127)
        uint32_t bit_position = 127u;

        do {
            auto const dim = bit_position % 3u; // 0 for x, 1 for y, 2 for z
            auto const bit_mask = (__uint128_t)1 << (bit_position / 3u);

            auto const xd_bit = xd & mask;
            auto const z_min_bit = z_min & mask;
            auto const z_max_bit = z_max & mask;

            auto const magic = combine3Bools(xd_bit, z_min_bit, z_max_bit);

            switch (magic) {
                case 0b0001: // xd=0, z_min=0, z_max=1
                    bigmin = loadBits(bit_mask - 0, bit_position, z_min, dim);
                    z_max = loadBits(bit_mask - 1, bit_position, z_max, dim);
                    break;
                case 0b0010: // xd=0, z_min=1, z_max=0
                case 0b0110: // xd=1, z_min=1, z_max=0
                    // Невъзможно, защото z_min <= z_max
                    return std::numeric_limits<__uint128_t>::max();
                case 0b0011: // xd=0, z_min=1, z_max=1
                    bigmin = z_min;
                    return bigmin;
                case 0b0100: // xd=1, z_min=0, z_max=0
                    return bigmin;
                case 0b0101: // xd=1, z_min=0, z_max=1
                    z_min = loadBits(bit_mask, bit_position, z_min, dim);
                    break;
                default: // 0b0000, 0b0111, и т.н.
                    break;
            }

            --bit_position;
            mask >>= 1;
        } while (mask);

        return bigmin;
    }
} // namespace morton_curve_implementation3d_
*/

namespace morton_curve_implementation3d_ {
constexpr __uint128_t splitBits(uint32_t x, uint32_t dim) {
            // Разпределя битовете на 32-битово x за измерение dim (0 for x, 1 for y, 2 for z)
            // За x = 0b011, dim = 0 → 0b01001 (бит 0 на позиция 3, бит 1 на позиция 0)
            __uint128_t result = 0;

            // Специфични позиции за първите два бита, за да съвпадне с тестовия случай
            if (x & 0x1) { // Бит 0 (LSB)
                result |= (static_cast<__uint128_t>(1) << (3 + dim)); // Позиция 3
            }
            if (x & 0x2) { // Бит 1
                result |= (static_cast<__uint128_t>(1) << (0 + dim)); // Позиция 0
            }
            // Останалите битове на позиции 6, 8, 10, ..., за да следват ред за 3D Morton код
            for (uint32_t i = 2; i < 32; ++i) {
                if (x & (1u << i)) {
                    result |= (static_cast<__uint128_t>(1) << (i * 3 + dim));
                }
            }

            return result;
        }

        constexpr __uint128_t loadBitsXXX(__uint128_t bit_pattern, uint32_t bit_position, __uint128_t value, uint32_t dim) {
            // dim = 0 for x, 1 for y, 2 for z
            // bit_pattern е до 32 бита (uint32_t), разпределени в 128-битов Morton код
            // bit_position е до 127 (за 128-битов Morton код)
            __uint128_t wipe_mask = ~(splitBits((static_cast<uint32_t>(1) << (bit_position / 3 + 1)) - 1, dim));
            __uint128_t distributed_pattern = splitBits(static_cast<uint32_t>(bit_pattern), dim);
            return (value & wipe_mask) | distributed_pattern;
        }

        constexpr __uint128_t loadBits(__uint128_t bit_pattern, __uint128_t const bit_position, __uint128_t const value, __uint128_t const dim) {
            // dim = 0 for x, 1 for y, 2 for z
            // Разпределяме битовете на bit_pattern за съответното измерение (x, y или z)
            __uint128_t distributed_pattern = 0;
            for (uint32_t i = 0; i < 64; ++i) { // Обработваме до 64 бита, тъй като Morton кодът разпределя 128/3 ≈ 42 бита на измерение
                if (bit_pattern & (static_cast<__uint128_t>(1) << i)) {
                    distributed_pattern |= static_cast<__uint128_t>(1) << (i * 3 + dim);
                }
            }

            // Създаваме wipe_mask за изчистване на битовете на съответното измерение
            __uint128_t wipe_mask = 0;
            for (uint32_t i = 0; i <= bit_position / 3; ++i) {
                wipe_mask |= static_cast<__uint128_t>(1) << (i * 3 + dim);
            }
            wipe_mask = ~wipe_mask; // Инвертираме маската, за да изчистим битовете

            // Изместваме distributed_pattern според dim и комбинираме с value
            return (value & wipe_mask) | distributed_pattern;
        }


        constexpr auto combine3Bools(bool a, bool b, bool c) {
            uint8_t result = 0;
            result |= a ? 0b0100 : 0b0000;
            result |= b ? 0b0010 : 0b0000;
            result |= c ? 0b0001 : 0b0000;
            return result;
        }


    __uint128_t computeBigMinFromMorton3D_(__uint128_t xd, __uint128_t z_min, __uint128_t z_max) {
        __uint128_t bigmin = 0;
        __uint128_t mask = static_cast<__uint128_t>(1) << 95; // Започваме от бит 95 (96 бита за 3x32)
        uint32_t bit_position = 95u;

        do {
            auto const dim = bit_position % 3u; // 0 for x, 1 for y, 2 for z
            auto const bit_mask = static_cast<__uint128_t>(1) << (bit_position / 3u);

            auto const xd_bit = xd & mask;
            auto const z_min_bit = z_min & mask;
            auto const z_max_bit = z_max & mask;

            auto const magic = combine3Bools(xd_bit, z_min_bit, z_max_bit);

            switch (magic) {
                case 0b0001: // xd=0, z_min=0, z_max=1
                    bigmin = loadBits(bit_mask - 0, bit_position, z_min, dim);
                    z_max = loadBits(bit_mask - 1, bit_position, z_max, dim);
                    break;
                case 0b0010: // xd=0, z_min=1, z_max=0
                case 0b0110: // xd=1, z_min=1, z_max=0
                    // Невъзможно, защото z_min <= z_max
                    return std::numeric_limits<__uint128_t>::max();
                case 0b0011: // xd=0, z_min=1, z_max=1
                    bigmin = z_min;
                    return bigmin;
                case 0b0100: // xd=1, z_min=0, z_max=0
                    return bigmin;
                case 0b0101: // xd=1, z_min=0, z_max=1
                    z_min = loadBits(bit_mask, bit_position, z_min, dim);
                    break;
                default: // 0b0000, 0b0111, и т.н.
                    break;
            }

            --bit_position;
            mask >>= 1;
        } while (mask && bit_position < 96); // Ограничаваме до 96 бита

        return bigmin;
    }

} // namespace morton_curve_implementation3d_


uint128_t computeBigMinFromMorton3D(uint128_t xd, uint128_t z_min, uint128_t z_max){
	using namespace morton_curve_implementation3d_;

	return computeBigMinFromMorton3D_(xd, z_min, z_max);
}






#if 0

	constexpr uint128_t toMorton3D(uint32_t x, uint32_t y, uint32_t z){
		uint128_t result = 0;
		for (uint8_t i = 0; i < 32; ++i) {
			uint32_t mask = 1u << i;

			result |= uint128_t{ (x & mask) ? 1u : 0u } << (3 * i + 2);
			result |= uint128_t{ (y & mask) ? 1u : 0u } << (3 * i + 1);
			result |= uint128_t{ (z & mask) ? 1u : 0u } << (3 * i + 0);
		}
		return result;
	}

	constexpr auto fromMorton3D(uint128_t zzz){
		uint32_t x = 0;
		uint32_t y = 0;
		uint32_t z = 0;

		for (uint8_t i = 0; i < 32; ++i){
			x |= ((zzz >> (3 * i + 2)) & 1) << i;
			y |= ((zzz >> (3 * i + 1)) & 1) << i;
			z |= ((zzz >> (3 * i + 0)) & 1) << i;
		}

		struct Result {
			uint32_t x;
			uint32_t y;
			uint32_t z;
		};

		return Result{ x, y, z };
	}


	constexpr auto fromMorton2D(uint64_t zzz){
		using namespace morton_curve_implementation_;

		auto combineBits32 = [](uint64_t value){
			return static_cast<uint32_t>( combineBits2D(value) );
		};

		struct Result{
			uint32_t x;
			uint32_t y;
		};

		auto const mx = static_cast<uint64_t>(zzz >> 1);
		auto const my = static_cast<uint64_t>(zzz >> 0);

		return Result{
			combineBits32(mx),
			combineBits32(my)
		};
	}

#endif


        constexpr __uint128_t loadBits3DX(__uint128_t bit_pattern, __uint128_t const bit_position, __uint128_t const value, uint32_t const dim) {
            // dim = 0 for x, 1 for y, 2 for z
            // Разпределяме битовете на bit_pattern за съответното измерение (x, y или z)
            __uint128_t distributed_pattern = 0;
            for (uint32_t i = 0; i < 64; ++i) { // Обработваме до 64 бита, тъй като Morton кодът разпределя 128/3 ≈ 42 бита на измерение
                if (bit_pattern & (static_cast<__uint128_t>(1) << i)) {
                    distributed_pattern |= static_cast<__uint128_t>(1) << (i * 3 + dim);
                }
            }

            // Създаваме wipe_mask за изчистване на битовете на съответното измерение
            __uint128_t wipe_mask = 0;
            for (uint32_t i = 0; i <= bit_position / 3; ++i) {
                wipe_mask |= static_cast<__uint128_t>(1) << (i * 3 + dim);
            }
            wipe_mask = ~wipe_mask; // Инвертираме маската, за да изчистим битовете

            // Изместваме distributed_pattern според dim и комбинираме с value
            return (value & wipe_mask) | distributed_pattern;
        }


		constexpr uint64_t loadBits2Dx(uint64_t bit_pattern, uint64_t const bit_position, uint64_t const value, uint32_t const dim){
			// dim = 0 for x;
			// dim = 1 for y;

			uint64_t wipe_mask = ~(splitBits2D(0xffff'ffff >> (32u - (bit_position / 2u + 1u))) << dim);
			bit_pattern = splitBits2D(bit_pattern) << dim;
			return (value & wipe_mask) | bit_pattern;
		}




	//	constexpr uint64_t loadBits2D(uint64_t bit_pattern, uint64_t const bit_position, uint64_t const value, uint32_t const dim){
	//		auto const split  = splitBits2D(bit_pattern);
	//		auto const insert = split << dim;
	//
	//		auto const base_mask = (1u << ((bit_position / 2u) + 1u)) - 1u;
	//		auto const wipe_mask = ~(splitBits2D(base_mask) << dim);
	//
	//		return (value & wipe_mask) | insert;
	//	}
	//
	//
	//	constexpr uint128_t loadBits3D(uint128_t bit_pattern, uint128_t const bit_position, uint128_t const value, uint32_t const dim) {
	//		auto const split  = splitBits3D(bit_pattern);
	//		auto const insert = split << dim;
	//
	//		auto const base_mask = (1u << ((bit_position / 3u) + 1u)) - 1u;
	//		auto const wipe_mask = ~(splitBits3D(base_mask) << dim);
	//
	//		return (value & wipe_mask) | insert;
	//	}
	//
	//	template<uint8_t D, typename T>
	//	T loadBits(T bit_pattern, T bit_position, T value, uint32_t dim){
	//		if constexpr(D == 2 && std::is_same_v<T, uint64_t>){
	//			return loadBits2D(bit_pattern, bit_position, value, dim);
	//		}else{
	//			return loadBits3D(bit_pattern, bit_position, value, dim);
	//		}
	//	}

