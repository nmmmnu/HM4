#ifndef MY_VECTORS_H_
#define MY_VECTORS_H_

#include <cassert>
#include <cstdint>
#include <cmath>
#include <limits>

#include "vectors_buffer.h"
#include "forcevectorize.h"

#include "murmur_hash_mixer.h"

namespace MyVectors{

	template<typename VE>
	constexpr bool checkVectorElement(){
		using T = std::remove_cv_t<VE>;

		return	std::is_same_v<T, float  > ||
			std::is_same_v<T, int8_t > ||
			std::is_same_v<T, int16_t >;
	}

	template<typename VE>
	constexpr bool checkFVectorElement(){
		using T = std::remove_cv_t<VE>;

		return	std::is_same_v<T, float>;
	}

	// ------------------------

	template<typename T>
	constexpr bool validBlobSize(size_t size, size_t dim){
		static_assert(checkVectorElement<T>(), "Only float, int8_t and int16_t supported");

		return size == dim * sizeof(T);
	}

	constexpr bool validBlobSizeF(size_t size, size_t dim){
		return validBlobSize<float>(size, dim);
	}

	struct DefaultValueProjection{
		template<typename T>
		constexpr auto const &operator()(T const &a){
			return a;
		}
	};

	// ------------------------

	template<typename T>
	constexpr float getMagnitude(TVector<T> const ctvector){
		static_assert(checkVectorElement<T>(), "Only float, int8_t and int16_t supported");

		float l2 = 0.0f;

		FORCE_VECTORIZE
		for(size_t i = 0; i < ctvector.size(); ++i)
			l2 += ctvector[i] * ctvector[i];

		return std::sqrt(l2);
	}

	// ------------------------

/*
	constexpr float normalizeInline(FVector fvector){
		constexpr float ZERO = 1E-6f;

		auto const magnitude = getMagnitude(fvector);

		if (magnitude < ZERO)
			return magnitude;

		auto const fix = 1 / magnitude;

		FORCE_VECTORIZE
		for(size_t i = 0; i < fvector.size(); ++i)
			fvector[i] *= fix;

		return magnitude;
	}
*/

	template<typename F>
	constexpr float normalizeF(CFVector const cfvector, F f){
		constexpr float ZERO = 1E-6f;

		auto const magnitude = getMagnitude(cfvector);

		if (magnitude < ZERO){
			// copy values
			for(size_t i = 0; i < cfvector.size(); ++i)
				f(i, 0.0f);

			return 0.0f;
		}else{
			auto const fix = 1 / magnitude;

			// normalize values
			for(size_t i = 0; i < cfvector.size(); ++i)
				f(i, cfvector[i] * fix);

			return magnitude;
		}
	}

	// ------------------------

	template<typename T>
	constexpr T quantizeComponent(float v){
		static_assert(checkVectorElement<T>(), "Only float, int8_t and int16_t supported");

		if constexpr(std::is_same_v<T, float>){
			return v;
		}

		if constexpr(std::is_same_v<T, int16_t>){
			float const scale = 32767;

			return static_cast<T>( std::round(v * scale) );
		}

		if constexpr(std::is_same_v<T, int8_t>){
			float const scale = 127;

			return static_cast<T>( std::round(v * scale) );
		}
	}

	// ------------------------

	template<typename T>
	constexpr int8_t quantizeComponentToI8(T v){
		static_assert(checkVectorElement<T>(), "Only float, int8_t and int16_t supported");

		if constexpr(std::is_same_v<T, float>){
			float const scale = 127;

			return static_cast<int8_t>( std::round(v * scale) );
		}

		if constexpr(std::is_same_v<T, int16_t> && 0){
			int16_t const maxI8  =   127;
			int16_t const maxI16 = 32767;
			return static_cast<int8_t>((v * maxI8) / maxI16);
		}

		if constexpr(std::is_same_v<T, int16_t> && 1){
			int8_t const minI8 = -128;
			int8_t const min   = -127;
			int8_t const res   = static_cast<int8_t>(v >> 8);

			return res == minI8 ? min : res;
		}

		if constexpr(std::is_same_v<T, int8_t>){
			return v;
		}
	}

	// ------------------------

	constexpr float dequantizeComponent(float v){
		return v;
	}

	constexpr float dequantizeComponent(int16_t v){
		float const scale = 1 / 32767.f;

		return v * scale;
	}

	constexpr float dequantizeComponent(int8_t v){
		float const scale = 1 / 127.f;

		return v * scale;
	}

	template<typename T, typename F, typename FProj = DefaultValueProjection>
	constexpr void dequantizeF(CTVector<T> const ctvector, F f, FProj fpr){
		static_assert(checkVectorElement<T>(), "Only float, int8_t and int16_t supported");

		FORCE_VECTORIZE
		for(size_t i = 0; i < ctvector.size(); ++i)
			f(i, dequantizeComponent(fpr(ctvector[i])));
	}

	// ------------------------

	template<typename T>
	constexpr float denormalizeComponent(T v, float const magnitude){
		static_assert(checkVectorElement<T>(), "Only float, int8_t and int16_t supported");

		return dequantizeComponent(v) * magnitude;
	}

/*
	template<typename T, typename FVector>
	constexpr void denormalizeInline(CTVector<T> const ctvector, float const magnitude, FVector fvector){
		static_assert(checkVectorElement<T>(), "Only float, int8_t and int16_t supported");

		assert(ctvector.size() == fvector.size());

		FORCE_VECTORIZE
		for(size_t i = 0; i < ctvector.size(); ++i)
			fvector[i] = denormalizeComponent(ctvector[i], magnitude);
	}
*/

	template<typename T, typename F, typename FProj = DefaultValueProjection>
	constexpr void denormalizeF(CTVector<T> const ctvector, float const magnitude, F f, FProj fpr){
		static_assert(checkVectorElement<T>(), "Only float, int8_t and int16_t supported");

		FORCE_VECTORIZE
		for(size_t i = 0; i < ctvector.size(); ++i)
			f(i, denormalizeComponent(fpr(ctvector[i]), magnitude));
	}

	// ------------------------

	namespace distance_cosine_impl_{

		template<typename T1, typename T2,
				typename FProj1 = DefaultValueProjection, typename FProj2 = DefaultValueProjection>
		float dotProduct(CTVector<T1> const a, CTVector<T2> const b,
									FProj1 fpr1, FProj2 fpr2){

			static_assert(checkVectorElement<T1>(), "Only float, int8_t and int16_t supported");
			static_assert(checkVectorElement<T2>(), "Only float, int8_t and int16_t supported");

			assert(a.size() == b.size());

			float dot = 0;

			FORCE_VECTORIZE
			for (size_t i = 0; i < a.size(); ++i){
				// dequantize float is a no op
				float const a_i = dequantizeComponent(fpr1(a[i]));
				float const b_i = dequantizeComponent(fpr2(b[i]));

				dot += a_i * b_i;
			}

			// returns -1.0 to +1.0

			return dot;
		}

		template<typename T1, typename T2,
				typename FProj1 = DefaultValueProjection, typename FProj2 = DefaultValueProjection>
		float cosineSimilarity(CTVector<T1> const a, CTVector<T2> const b,
									FProj1 aFpr, FProj2 bFpr){

			static_assert(checkVectorElement<T1>(), "Only float, int8_t and int16_t supported");
			static_assert(checkVectorElement<T2>(), "Only float, int8_t and int16_t supported");

			auto const dot = dotProduct(a, b, aFpr, bFpr);

			auto const result = (1 + dot) / 2;

			constexpr float ZERO = 1E-6f;

			// returns +1.0 to 0.0

			return result > ZERO ? result : 0;
		}

	} // namespace distance_cosine_impl_

	template<typename T1, typename T2,
				typename FProj1 = DefaultValueProjection, typename FProj2 = DefaultValueProjection>
	float distanceCosine(CTVector<T1> const a, CTVector<T2> const b,
									FProj1 aFpr, FProj2 bFpr){

		static_assert(checkVectorElement<T1>(), "Only float, int8_t and int16_t supported");
		static_assert(checkVectorElement<T2>(), "Only float, int8_t and int16_t supported");

		using namespace distance_cosine_impl_;

		// returns 0.0 to +1.0

		return 1 - cosineSimilarity(a, b, aFpr, bFpr);
	}

	template<typename T1, typename T2,
				typename FProj1 = DefaultValueProjection, typename FProj2 = DefaultValueProjection>
	float distanceEuclideanSquared(CTVector<T1> const a, CTVector<T2> const b, float aM, float bM,
									FProj1 aFpr, FProj2 bFpr){

		static_assert(checkVectorElement<T1>(), "Only float, int8_t and int16_t supported");
		static_assert(checkVectorElement<T2>(), "Only float, int8_t and int16_t supported");

		using namespace distance_cosine_impl_;

		auto const dot = dotProduct(a, b, aFpr, bFpr);

		auto const result = aM * aM + bM * bM - 2 * aM * bM * dot;

		constexpr float ZERO = 1E-6f;

		// returns 0.0 to INF

		return result > ZERO ? result : 0;
	}

	template<typename T1, typename T2,
				typename FProj1 = DefaultValueProjection, typename FProj2 = DefaultValueProjection>
	float distanceEuclidean(CTVector<T1> const a, CTVector<T2> const b, float aM, float bM,
									FProj1 aFpr, FProj2 bFpr){

		static_assert(checkVectorElement<T1>(), "Only float, int8_t and int16_t supported");
		static_assert(checkVectorElement<T2>(), "Only float, int8_t and int16_t supported");

		auto const result = distanceEuclideanSquared(a, b, aM, bM, aFpr, bFpr);

		// returns 0.0 to INF

		return std::sqrt(result);
	}

	template<typename T1, typename T2,
				typename FProj1 = DefaultValueProjection, typename FProj2 = DefaultValueProjection>
	float distanceCanberra(CTVector<T1> const a, CTVector<T2> const b, float aM, float bM,
									FProj1 aFpr, FProj2 bFpr){

		static_assert(checkVectorElement<T1>(), "Only float, int8_t and int16_t supported");
		static_assert(checkVectorElement<T2>(), "Only float, int8_t and int16_t supported");

		float result = 0.0f;

		for (size_t i = 0; i < a.size(); ++i) {
			float const a_i = denormalizeComponent(aFpr(a[i]), aM);
			float const b_i = denormalizeComponent(bFpr(b[i]), bM);

			constexpr float ZERO = 1E-6f;

			if (float const den = std::abs(a_i) + std::abs(b_i); den > ZERO)
				result += std::abs(a_i - b_i) / den;
		}

		// returns 0.0 to INF

		return result;
	}

	template<typename T2,
				typename FProj1 = DefaultValueProjection, typename FProj2 = DefaultValueProjection>
	float distanceCanberraPrepared(CFVector const a, CTVector<T2> const b, float bM,
									FProj1 aFpr, FProj2 bFpr){

		static_assert(checkVectorElement<T2>(), "Only float, int8_t and int16_t supported");

		float result = 0.0f;

		for (size_t i = 0; i < a.size(); ++i) {
			float const a_i =                      aFpr(a[i])     ;
			float const b_i = denormalizeComponent(bFpr(b[i]), bM);

			constexpr float ZERO = 1E-6f;

			if (float const den = std::abs(a_i) + std::abs(b_i); den > ZERO)
				result += std::abs(a_i - b_i) / den;
		}

		// returns 0.0 to INF

		return result;
	}

	template<typename T1, typename T2,
				typename FProj1 = DefaultValueProjection, typename FProj2 = DefaultValueProjection>
	float distanceManhattan(CTVector<T1> const a, CTVector<T2> const b, float aM, float bM,
									FProj1 aFpr, FProj2 bFpr){

		static_assert(checkVectorElement<T1>(), "Only float, int8_t and int16_t supported");
		static_assert(checkVectorElement<T2>(), "Only float, int8_t and int16_t supported");

		float result = 0.0f;

		for (size_t i = 0; i < a.size(); ++i) {
			float const a_i = denormalizeComponent(aFpr(a[i]), aM);
			float const b_i = denormalizeComponent(bFpr(b[i]), bM);

			result += std::abs(a_i - b_i);
		}

		// returns 0.0 to INF

		return result;
	}

	template<typename T2,
				typename FProj1 = DefaultValueProjection, typename FProj2 = DefaultValueProjection>
	float distanceManhattanPrepared(CFVector const a, CTVector<T2> const b, float bM,
									FProj1 aFpr, FProj2 bFpr){

		static_assert(checkVectorElement<T2>(), "Only float, int8_t and int16_t supported");

		float result = 0.0f;

		for (size_t i = 0; i < a.size(); ++i) {
			float const a_i =                      aFpr(a[i])     ;
			float const b_i = denormalizeComponent(bFpr(b[i]), bM);

			result += std::abs(a_i - b_i);
		}

		// returns 0.0 to INF

		return result;
	}

	// ------------------------

	namespace MD{

		template<typename T>
		T distribution(uint64_t a64);

		template<>
		constexpr float distribution<float>(uint64_t a){
			float const scale  = 1.0f / static_cast<float>(std::numeric_limits<uint64_t>::max());
			float const scale2 = scale * 2;

			return static_cast<float>(a) * scale2 - 1.0f;
		}

		template<>
		constexpr int8_t distribution<int8_t>(uint64_t a){
			int8_t const minI8 = -128;
			int8_t const min   = -127;
			int8_t const res   = static_cast<int8_t>(a);

			return res == minI8 ? min : res;
		}

	}

	void randomProjection(CFVector const cfvector, FVector fresult, uint64_t seed = 0){
		MurmurHashMixer64 generator{ seed };

		for (size_t x = 0; x < fresult.size(); ++x){
			float sum = 0.0f;

			for (size_t y = 0; y < cfvector.size(); ++y)
				sum += cfvector[y] * MD::distribution<float>(generator());

			fresult[x] = sum;
		}
	}

/*
	void randomProjectionNormalize(CFVector const cfvector, FVector fresult){

		randomProjection(cfvector, fresult);

		normalizeInline(fresult);
	}
*/

	template <size_t MaxDimensions, typename T, typename Generator>
	struct MultiHyperplaneProjector{
		static_assert(
			std::is_same_v<T, uint8_t > ||
			std::is_same_v<T, uint16_t> ||
			std::is_same_v<T, uint32_t> ||
			std::is_same_v<T, uint64_t>
		);

		constexpr MultiHyperplaneProjector(CTVector<int8_t> vector, size_t bits, uint64_t seed = 0) :
								vector_		(vector	),
								bits_		(bits	),
								generator_	(seed	){
			assert(bits_ <= MaxBits);
		}

		constexpr T operator()(){
			uint64_t random64[MaxCapacity64]; // for 8K -> 1K

			// 1. Generate random numbers (bits)
			const T *randomT = generateRandom_(random64);

			// 2. Single pass simplified dot product of all bits
			int32_t dots[MaxBits]{};

			for (size_t i = 0; i < vector_.size(); ++i){
				auto const v	= vector_[i];

				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
				auto const mask	= randomT[i];
				#pragma GCC diagnostic pop

				// cycle to MaxBits is *way* faster,
				// than cycle to bits_

				FORCE_VECTORIZE
				for (size_t h = 0; h < MaxBits; ++h){
					const bool bit = (mask >> h) & 1;
					dots[h] += bit ? +v : -v;
				}
			}

			// 3. Result
			T result = 0;

			for (size_t i = 0; i < bits_; ++i)
				if (dots[i] > 0)
					result |= static_cast<T>(1u << i);

			return result;
		}

	private:
		constexpr const T *generateRandom_(uint64_t *random64){
			size_t const needed64 = size64__(vector_.size());

			for (size_t i = 0; i < needed64; ++i)
				random64[i] = generator_();

			return reinterpret_cast<const T *>(random64);
		}

		constexpr static size_t size64__(size_t size){
			return (size * sizeof(T) + sizeof(uint64_t) - 1) / sizeof(uint64_t);
		}

	private:
		constexpr static size_t MaxBits		= sizeof(T) * 8;
		constexpr static size_t MaxCapacity64	= size64__(MaxDimensions);

	private:
		CTVector<int8_t>	vector_;
		size_t			bits_;
		Generator		generator_;
	};

	template<size_t MaxDimensions, typename T, typename F>
	void simhashBands(CTVector<int8_t> const vector, uint8_t bits, uint8_t bands, F f, uint64_t seed = 0){
		MultiHyperplaneProjector<MaxDimensions, T, MurmurHashMixer64> mhpp{ vector, bits, seed };

		for(uint8_t id = 0; id < bands; ++id)
			f(id, mhpp());
	}

} // namspace MyVectors

#endif // MY_VECTORS_H_


