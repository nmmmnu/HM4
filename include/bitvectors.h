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

	size_t distanceHamming(std::string_view a, std::string_view b){
		assert(a.size() == b.size()	&& "Size of the vectors must be the same");

		size_t result = 0;

		// const auto *pa = reinterpret_cast<const uint8_t *>(a.data());
		// const auto *pb = reinterpret_cast<const uint8_t *>(b.data());
		//
		// for (size_t i = 0; i < a.size(); ++i)
		// 	result += __builtin_popcount(pa[i] ^ pb[i]);



		size_t const size64 = a.size() / sizeof(uint64_t);

		const uint64_t *pa64 = reinterpret_cast<const uint64_t *>(a.data());
		const uint64_t *pb64 = reinterpret_cast<const uint64_t *>(b.data());

		#pragma GCC ivdep
		for (size_t i = 0; i < size64; ++i)
		    result += __builtin_popcountll(pa64[i] ^ pb64[i]);

		const uint8_t  *pa8  = reinterpret_cast<const uint8_t *>(pa64 + size64);
		const uint8_t  *pb8  = reinterpret_cast<const uint8_t *>(pb64 + size64);

		for (size_t i = 0; i < a.size() % sizeof(uint64_t); ++i)
		    result += __builtin_popcount(pa8[i] ^ pb8[i]);



		return result;
	}

	float distanceHammingFloat(std::string_view a, std::string_view b){
		return static_cast<float>(
			distanceHamming(a, b)
		);
	}

	float distanceCosineBit(std::string_view a, std::string_view b){
		assert(a.size() == b.size()	&& "Size of the vectors must be the same");

		size_t dot   = 0;
		size_t normA = 0;
		size_t normB = 0;



		size_t const size64 = a.size() / sizeof(uint64_t);

		const uint64_t *pa64 = reinterpret_cast<const uint64_t *>(a.data());
		const uint64_t *pb64 = reinterpret_cast<const uint64_t *>(b.data());

		#pragma GCC ivdep
		for (size_t i = 0; i < size64; ++i){
			auto const byteA = pa64[i];
			auto const byteB = pb64[i];

			dot   += __builtin_popcountll(byteA & byteB);
			normA += __builtin_popcountll(byteA);
			normB += __builtin_popcountll(byteB);
		}

		const uint8_t  *pa8  = reinterpret_cast<const uint8_t *>(pa64 + size64);
		const uint8_t  *pb8  = reinterpret_cast<const uint8_t *>(pb64 + size64);

		for (size_t i = 0; i < a.size() % sizeof(uint64_t); ++i){
			auto const byteA = pa8[i];
			auto const byteB = pb8[i];

			dot   += __builtin_popcount(byteA & byteB);
			normA += __builtin_popcount(byteA);
			normB += __builtin_popcount(byteB);
		}



		if (normA == 0 || normB == 0)
			return 1.0;

		return 1 - static_cast<float>(dot * dot) / static_cast<float>(normA * normB);
	}

	size_t distanceDominatingPrepare(std::string_view a){
		size_t result = 0;



		size_t const size64 = a.size() / sizeof(uint64_t);

		const uint64_t *pa64 = reinterpret_cast<const uint64_t *>(a.data());

		#pragma GCC ivdep
		for (size_t i = 0; i < size64; ++i)
		    result += __builtin_popcountll(pa64[i]);

		const uint8_t  *pa8  = reinterpret_cast<const uint8_t *>(pa64 + size64);

		for (size_t i = 0; i < a.size() % sizeof(uint64_t); ++i)
		    result += __builtin_popcount(pa8[i]);



		return result;
	}

	float distanceDominatingPrepared(std::string_view a, std::string_view b){
		assert(a.size() == b.size()	&& "Size of the vectors must be the same");

		size_t result = 0;



		size_t const size64 = a.size() / sizeof(uint64_t);

		const uint64_t *pa64 = reinterpret_cast<const uint64_t *>(a.data());
		const uint64_t *pb64 = reinterpret_cast<const uint64_t *>(b.data());

		#pragma GCC ivdep
		for (size_t i = 0; i < size64; ++i)
		    result += __builtin_popcountll(pa64[i] & pb64[i]);

		const uint8_t  *pa8  = reinterpret_cast<const uint8_t *>(pa64 + size64);
		const uint8_t  *pb8  = reinterpret_cast<const uint8_t *>(pb64 + size64);

		for (size_t i = 0; i < a.size() % sizeof(uint64_t); ++i)
		    result += __builtin_popcount(pa8[i] & pb8[i]);



		// return 1 - static_cast<float>(result) / static_cast<float>(popA);

		// bigger popcount, smaller distance
		return - static_cast<float>(result);
	}

} // namspace MyVectors

#endif // MY_VECTORS_H_

