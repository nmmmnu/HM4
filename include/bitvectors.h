#ifndef MY_BIT_VECTORS_H_
#define MY_BIT_VECTORS_H_

#include <cassert>
#include <cstdint>

#include <string_view>

#include "forcevectorize.h"
#include "mybitbufferview.h"

#include "mypopcount.h"

namespace MyVectors{

	using BVector = MyBuffer::BitBufferView;

//	constexpr bool bitVectorGetComponent(const uint8_t *data, size_t index){
//		size_t  const byte  = index / 8;
//		size_t  const bit   = index % 8;
//		uint8_t const value = data[byte];
//
//		return (value >> bit) & 1;
//	}

//	constexpr size_t bitVectorBytes(size_t size){
//		return (size + 7) / 8;
//	}

	// ------------------------

	constexpr uint8_t bitVectorQuantizeComponent(float v){
		return v > 0 ? 1 : 0;
	}

	template <typename CFVector>
	constexpr void bitVectorQuantize(CFVector const &vector, uint8_t *output){
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

	// ------------------------

	float distanceHamming(BVector a, BVector b){
		assert(a.bits() == b.bits() && "Size of the vectors must be the same");

		return static_cast<float>(
			MyPopcount::popcountXOR(a.toSV(), b.toSV(), a.bits())
		);
	}

	constexpr float distanceHammingPrepareFix(BVector a){
		if (a.bits())
			return 1.f / static_cast<float>(a.bits());
		else
			return 0.f;
	}

	constexpr float distanceHammingFix(float distance, float fix){
		return distance * fix;
	}

	// ------------------------

	constexpr float distanceCosineBitPrepareFix(BVector){
		return 0;
	}

	float distanceCosineBit(BVector a, BVector b){
		assert(a.bits() == b.bits() && "Size of the vectors must be the same");

		auto [dot, normA, normB] = MyPopcount::popcountDOT(a.toSV(), b.toSV(), a.bits());

		if (normA == 0 || normB == 0)
			return 1.f;

		return 1.f - static_cast<float>(dot * dot) / static_cast<float>(normA * normB);
	}

	// constexpr float distanceCosineBitFix(float distance, float /* fix */){
	// 	return std::sqrt(std::max(0.f, distance));
	// }

	constexpr float distanceCosineBitFix(float distance, float /* fix */){
		return distance;
	}

	// ------------------------

	float distanceDominatingPrepareFix(BVector a){
		if (auto const pc = MyPopcount::popcount(a.toSV(), a.bits()); pc)
			return 1.f / static_cast<float>(pc);
		else
			return 0.f;
	}

	float distanceDominatingPrepared(BVector a, BVector b){
		assert(a.bits() == b.bits() && "Size of the vectors must be the same");

		auto const pc = MyPopcount::popcountAND(a.toSV(), b.toSV(), a.bits());

		// bigger popcount, smaller distance
		return - static_cast<float>(pc);
	}

	constexpr float distanceDominatingFix(float distance, float fix){
		return 1.f + distance * fix;
	}



	// ------------------------

	namespace simhash_impl_{

		template <size_t MaxDimensions, typename HashType, typename Generator>
		struct MultiHyperplaneProjectorBit{
			static_assert(
				std::is_same_v<HashType, uint8_t > ||
				std::is_same_v<HashType, uint16_t> ||
				std::is_same_v<HashType, uint32_t> ||
				std::is_same_v<HashType, uint64_t>
			);

			constexpr MultiHyperplaneProjectorBit(BVector vector, uint64_t seed = 0) :
									vector_		(vector	),
									generator_	(seed	){}

			constexpr HashType operator()(){
				uint64_t random64[MaxCapacity64]; // for 8K -> 1K

				HashType result = 0;

				for (size_t i = 0; i < MaxBits; ++i){
					// 1. Generate random numbers (bits)
					generateRandom_(random64);

					BVector rvector{
						random64,
						vector_.bits()
					};

					// 2. generate hamming
					auto const h   = MyPopcount::popcountXOR(vector_.toSV(), rvector.toSV(), vector_.bits());

					// 3. Result
					bool const dot = h <= vector_.bits() / 2;

					if (dot)
						result |= static_cast<HashType>(1u << i);
				}

				return result;
			}

		private:
			constexpr void generateRandom_(uint64_t *random64){
				size_t const needed64 = size64__(vector_.bits());

				for (size_t i = 0; i < needed64; ++i)
					random64[i] = generator_();
			}

			constexpr static size_t size64__(size_t bits){
				auto const bits64 = sizeof(uint64_t) * 8;

				return (bits + bits64 - 1) / bits64;
			}

		private:
			constexpr static size_t MaxBits		= sizeof(HashType) * 8;
			constexpr static size_t MaxCapacity64	= size64__(MaxDimensions);

		private:
			BVector		vector_;
			Generator	generator_;
		};

	} // namespace simhash_impl_

	template<size_t MaxDimensions, typename HashType, typename F>
	void simhashBandsBit(BVector vector, size_t bands, F f, uint64_t seed = 0){
		using namespace simhash_impl_;

		assert(bands <= std::numeric_limits<uint8_t>::max() + 1);

		MultiHyperplaneProjectorBit<MaxDimensions, HashType, MurmurHashMixer64> mhpp{ vector, seed };

		for(size_t id = 0; id < bands; ++id)
			f(id, mhpp());
	}

} // namspace MyVectors

#endif // MY_VECTORS_H_

