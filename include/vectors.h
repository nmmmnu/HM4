#ifndef MY_VECTORS_H_
#define MY_VECTORS_H_

#include <cassert>
#include <cstdint>
#include <cmath>
#include <limits>

#include "murmur_hash_mixer.h"

namespace MyVectors{
	template<typename T2>
	constexpr bool checkVectorElement(){
		using T = std::remove_cv_t<T2>;

		return	std::is_same_v<T, float		> ||
			std::is_same_v<T, int8_t	>;
	}

	template<typename Vector>
	constexpr bool checkVector(){
		return checkVectorElement<typename Vector::value_type>();
	}

	template<typename VectorContainer>
	constexpr bool checkFVector(){

		using T = std::remove_cv_t<typename VectorContainer::value_type>;

		return	std::is_same_v<T, float>;
	}

	// ------------------------

	template<typename T>
	constexpr bool validBlobSize(size_t size, size_t dim){
		static_assert(checkVectorElement<T>(), "Only float and int8_t supported");

		return size == dim * sizeof(T);
	}

	constexpr bool validBlobSizeF(size_t size, size_t dim){
		return validBlobSize<float>(size, dim);
	}

	// ------------------------

	template<typename CVector>
	constexpr float getMagnitude(CVector const &cvector){
		static_assert(checkVector<CVector>(), "Only float and int8_t supported");

		float l2 = 0.0f;

		#pragma GCC ivdep
		for(size_t i = 0; i < cvector.size(); ++i)
			l2 += cvector[i] * cvector[i];

		return std::sqrt(l2);
	}

	// ------------------------

	template<typename FVector>
	constexpr float normalizeInline(FVector &fvector){
		static_assert(checkFVector<FVector>(), "Only float supported");

		constexpr float ZERO = 1E-6f;

		auto const magnitude = getMagnitude(fvector);

		if (magnitude < ZERO)
			return magnitude;

		#pragma GCC ivdep
		for(size_t i = 0; i < fvector.size(); ++i)
			fvector[i] /= magnitude;

		return magnitude;
	}

	template<typename CFVector, typename F>
	constexpr float normalizeF(CFVector const &cfvector, F f){
		static_assert(checkFVector<CFVector>(), "Only float supported");

		constexpr float ZERO = 1E-6f;

		auto const magnitude = getMagnitude(cfvector);

		if (magnitude < ZERO){
			// copy values
			for(size_t i = 0; i < cfvector.size(); ++i)
				f(i, 0.0f);

			return 0.0f;
		}else{
			// normalize values
			for(size_t i = 0; i < cfvector.size(); ++i)
				f(i, cfvector[i] / magnitude);

			return magnitude;
		}
	}

	// ------------------------

	template<typename T>
	constexpr T quantizeComponent(float v){
		static_assert(checkVectorElement<T>(), "Only float and int8_t supported");

		if (std::is_same_v<T, float>){
			return v;
		}

		if (std::is_same_v<T, int8_t>){
			float const scale = 127;

			return static_cast<int8_t>( std::round(v * scale) );
		}
	}

	// ------------------------

	constexpr float dequantizeComponent(float v){
		return v;
	}

	constexpr float dequantizeComponent(int8_t v){
		float const scale = 1 / 127.f;

		return v * scale;
	}

	template<typename CVector, typename F>
	constexpr void dequantizeF(CVector const &cvector, F f){
		static_assert(checkVector <CVector>(), "Only float and int8_t supported");

		#pragma GCC ivdep
		for(size_t i = 0; i < cvector.size(); ++i)
			f(i, dequantizeComponent(cvector[i]));
	}

	// ------------------------

	template<typename T>
	constexpr float denormalizeComponent(T v, float const magnitude){
		static_assert(checkVectorElement<T>(), "Only float and int8_t supported");

		return dequantizeComponent(v) * magnitude;
	}

	template<typename CVector, typename FVector>
	constexpr void denormalizeInline(CVector const &cvector, float const magnitude, FVector &fvector){
		static_assert(checkVector <CVector>(), "Only float and int8_t supported");
		static_assert(checkFVector<FVector>(), "Only float supported");

		assert(cvector.size() == fvector.size());

		#pragma GCC ivdep
		for(size_t i = 0; i < cvector.size(); ++i)
			fvector[i] = denormalizeComponent(cvector[i], magnitude);
	}

	template<typename CVector, typename F>
	constexpr void denormalizeF(CVector const &cvector, float const magnitude, F f){
		static_assert(checkVector<CVector>(), "Only float and int8_t supported");

		#pragma GCC ivdep
		for(size_t i = 0; i < cvector.size(); ++i)
			f(i, denormalizeComponent(cvector[i], magnitude));
	}

	// ------------------------

	namespace distance_cosine_impl_{

		template<typename CVector1, typename CVector2>
		float dotProduct(CVector1 const &a, CVector2 const &b){
			static_assert(checkVector<CVector1>(), "Only float and int8_t supported");
			static_assert(checkVector<CVector2>(), "Only float and int8_t supported");

			assert(a.size() == b.size());

			float dot = 0;

			#pragma GCC ivdep
			for (size_t i = 0; i < a.size(); ++i){
				// dequantize float is a no op
				float const a_i = dequantizeComponent(a[i]);
				float const b_i = dequantizeComponent(b[i]);

				dot += a_i * b_i;
			}

			return dot;
		}

		template<typename CVector1, typename CVector2>
		float cosineSimilarity(CVector1 const &a, CVector2 const &b){
			static_assert(checkVector<CVector1>(), "Only float and int8_t supported");
			static_assert(checkVector<CVector2>(), "Only float and int8_t supported");

			auto const dot = dotProduct(a, b);

			// returns 0.0 - 1.0

			auto const result = (1 + dot) / 2;

			constexpr float ZERO = 1E-6f;

			return result > ZERO ? result : 0;
		}

	} // namespace distance_cosine_impl_

	template<typename CVector1, typename CVector2>
	float distanceCosine(CVector1 const &a, CVector2 const &b, float /* aM */, float /* bM */){
		static_assert(checkVector<CVector1>(), "Only float and int8_t supported");
		static_assert(checkVector<CVector2>(), "Only float and int8_t supported");

		using namespace distance_cosine_impl_;

		return 1 - cosineSimilarity(a, b);
	}

	template<typename CVector1, typename CVector2>
	float distanceEuclideanSquared(CVector1 const &a, CVector2 const &b, float aM, float bM){
		static_assert(checkVector<CVector1>(), "Only float and int8_t supported");
		static_assert(checkVector<CVector2>(), "Only float and int8_t supported");

		using namespace distance_cosine_impl_;

		auto const dot = dotProduct(a, b);

		// returns 0.0 - INF

		auto const result = aM * aM + bM * bM - 2 * aM * bM * dot;

		constexpr float ZERO = 1E-6f;

		return result > ZERO ? result : 0;
	}

	template<typename CVector1, typename CVector2>
	float distanceEuclidean(CVector1 const &a, CVector2 const &b, float aM, float bM){
		static_assert(checkVector<CVector1>(), "Only float and int8_t supported");
		static_assert(checkVector<CVector2>(), "Only float and int8_t supported");

		auto const result = distanceEuclideanSquared(a, b, aM, bM);

		return std::sqrt(result);
	}

	template<typename CVector1, typename CVector2>
	float distanceCanberra(CVector1 const &a, CVector2 const &b, float /* aM */, float /* bM */){
		static_assert(checkVector<CVector1>(), "Only float and int8_t supported");
		static_assert(checkVector<CVector2>(), "Only float and int8_t supported");

		float result = 0.0f;

		for (size_t i = 0; i < a.size(); ++i) {
			// dequantize float is a no op
			float const a_i = dequantizeComponent(a[i]);
			float const b_i = dequantizeComponent(b[i]);

			constexpr float ZERO = 1E-6f;

			if (float const den = std::abs(a_i) + std::abs(b_i); den > ZERO)
				result += std::abs(a_i - b_i) / den;
		}

		return result;
	}

	template<typename CVector1, typename CVector2>
	float distanceManhattan(CVector1 const &a, CVector2 const &b, float aM, float bM){
		static_assert(checkVector<CVector1>(), "Only float and int8_t supported");
		static_assert(checkVector<CVector2>(), "Only float and int8_t supported");

		float result = 0.0f;

		for (size_t i = 0; i < a.size(); ++i) {
			float const a_i = denormalizeComponent(a[i], aM);
			float const b_i = denormalizeComponent(b[i], bM);

			result += std::abs(a_i - b_i);
		}

		return result;
	}

	template<typename CFVector1, typename CVector2>
	float distanceManhattanPrepared(CFVector1 const &a, CVector2 const &b, float bM){
		static_assert(checkFVector<CFVector1>(), "Only float supported");
		static_assert(checkVector <CVector2 >(), "Only float and int8_t supported");

		float result = 0.0f;

		for (size_t i = 0; i < a.size(); ++i) {
			float const a_i =                      a[i]     ;
			float const b_i = denormalizeComponent(b[i], bM);

			result += std::abs(a_i - b_i);
		}

		return result;
	}

	// ------------------------

	namespace random_projection_impl_{
		constexpr uint32_t M(uint32_t x, uint32_t y) {
			uint64_t const result = (uint64_t{x} << 32) | y;

			return static_cast<uint32_t>(
					murmur_hash_mixer64_nz(result)
			);
		}

		constexpr float scale  = 1.0f / static_cast<float>(std::numeric_limits<uint32_t>::max());
		constexpr float scale2 = scale * 2;

		constexpr float distribution(uint32_t h){
			return static_cast<float>(h) * scale2 - 1.f;
		}

		constexpr float MD(size_t x, size_t y, size_t seed = 0){
			return distribution(
				M(
					static_cast<uint32_t>(x + seed),
					static_cast<uint32_t>(y + seed)
				)
			);
		}
	}

	template<typename CFVector, typename FVector>
	void randomProjection(CFVector const &cfvector, FVector &fresult, size_t seed = 0){
		static_assert(checkFVector<CFVector>(), "Only float supported");
		static_assert(checkFVector<FVector >(), "Only float supported");

		using namespace random_projection_impl_;

		for (size_t x = 0; x < fresult.size(); ++x){
			float sum = 0.0f;

			#pragma GCC ivdep
			for (size_t y = 0; y < cfvector.size(); ++y)
				sum += cfvector[y] * MD(x, y, seed);

			fresult[x] = sum;
		}
	}

	template<typename CFVector, typename FVector>
	void randomProjectionNormalize(CFVector const &cfvector, FVector &fresult){
		static_assert(checkFVector<CFVector>(), "Only float supported");
		static_assert(checkFVector<FVector >(), "Only float supported");

		randomProjection(cfvector, fresult);

		normalizeInline(fresult);
	}

	template<typename T, typename CFVector>
	T randomProjectionBit(CFVector const &cfvector, size_t seed = 0){
		static_assert(checkFVector<CFVector>(), "Only float supported");

		using namespace random_projection_impl_;

		constexpr auto bits = sizeof(T) * 8;

		T result = 0;

		for (size_t x = 0; x < bits; ++x){
			float sum = 0.0f;

			#pragma GCC ivdep
			for (size_t y = 0; y < cfvector.size(); ++y)
				sum += cfvector[y] * MD(x, y, seed);

			T const I = sum > 0 ? 1 : 0;

			result |= I << x;
		}

		return result;
	}

} // namspace MyVectors

#endif // MY_VECTORS_H_


