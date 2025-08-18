#ifndef MY_BIT_VECTORS_H_
#define MY_BIT_VECTORS_H_

#include <cassert>
#include <cstdint>

#include <string_view>

namespace MyVectors{

	constexpr bool bitVectorGetComponent(const uint8_t *data, size_t index){
		size_t  const byte  = index / 8;
		size_t  const bit   = index % 8;
		uint8_t const value = data[byte];

		return (value >> bit) & 1;
	}

	constexpr uint8_t bitVectorQuantizeComponent(float v){
		return v > 0 ? 1 : 0;
	}

	template <typename FVector>
	constexpr void bitVectorQuantize(FVector const &vector, uint8_t *output){
		size_t i   = 0;
		size_t out = 0;

		size_t const size = vector.size();

		while (i < size){
			uint8_t byte = 0;

			for (size_t j = 0; j < 8; ++j){
				if (i + j >= size)
					break;

				uint8_t const bit = bitVectorQuantizeComponent(vector[i + j]);

				byte |= bit << j;
			}

			output[out++] = byte;

			i += 8;
		}
	}

	constexpr size_t bitVectorBytes(size_t size){
		return (size + 7) / 8;
	}

	size_t distanceHamming(std::string_view a, std::string_view b) {
		assert(a.size() == b.size());

		const auto *pa = reinterpret_cast<const uint8_t *>(a.data());
		const auto *pb = reinterpret_cast<const uint8_t *>(b.data());

		size_t result = 0;

		for (size_t i = 0; i < a.size(); ++i)
			result += __builtin_popcount(pa[i] ^ pb[i]);

		return result;
	}

} // namspace MyVectors

#endif // MY_VECTORS_H_

