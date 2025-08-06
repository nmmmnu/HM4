#ifndef MY_VECTORS_H_
#define MY_VECTORS_H_

#include <cassert>
#include <cstdint>
#include <cmath>
#include <limits>

#include "mybufferview.h"
#include "murmur_hash_mixer.h"

namespace MyVectors{
	template<typename T>
	using Vector  = MyBuffer::BufferView<T>;

	using FVector = Vector<float  >;
	using IVector = Vector<int8_t >;

	template<typename T>
	using VectorC  = MyBuffer::BufferView<T const>;

	using FVectorC = VectorC<float  >;
	using IVectorC = VectorC<int8_t >;

	// ------------------------

	template<typename T1>
	constexpr bool checkT(){
		using T = std::remove_cv_t<T1>;

		return 	std::is_same_v<T, float		> ||
			std::is_same_v<T, int8_t	>;
	}

	// ------------------------

	template<typename T>
	constexpr bool validBlobSize(size_t size, size_t dim){
		static_assert(checkT<T>(), "Only float and int8_t supported");

		return size == dim * sizeof(T);
	}

	constexpr bool validBlobSizeF(size_t size, size_t dim){
		return validBlobSize<float>(size, dim);
	}

	// ------------------------

	constexpr float getMagnitude(FVectorC const &vector){
		float l2 = 0.0f;

		for(size_t i = 0; i < vector.size(); ++i)
			l2 += vector[i] * vector[i];

		return std::sqrt(l2);
	}

	// ------------------------

	constexpr float normalizeInline(FVector &vector){
		constexpr float ZERO = 1E-6f;

		auto const magnitude = getMagnitude(vector);

		if (magnitude < ZERO)
			return magnitude;

		for(size_t i = 0; i < vector.size(); ++i)
			vector[i] /= magnitude;

		return magnitude;
	}

	template<typename F>
	constexpr float normalizeF(FVectorC const &vector, F f){
		constexpr float ZERO = 1E-6f;

		auto const magnitude = getMagnitude(vector);

		if (magnitude < ZERO){
			// copy values
			for(size_t i = 0; i < vector.size(); ++i)
				f(i, 0.0f);

			return 0.0f;
		}else{
			// normalize values
			for(size_t i = 0; i < vector.size(); ++i)
				f(i, vector[i] / magnitude);

			return magnitude;
		}
	}

	// ------------------------

	namespace quantize_component_impl_{
		constexpr float quantizeFP32(float v){
			return v;
		}

		constexpr int8_t quantizeI8(float v){
			float const scale = 127;

			return static_cast<int8_t>(
					std::round(v * scale)
			);
		}
	}

	template<typename T>
	constexpr T quantizeComponent(float v){
		static_assert(checkT<T>(), "Only float and int8_t supported");

		using namespace quantize_component_impl_;

		if (std::is_same_v<T, float>)
			return quantizeFP32(v);

		if (std::is_same_v<T, int8_t>)
			return quantizeI8(v);
	}

	// ------------------------

	constexpr float dequantizeComponent(float v){
		return v;
	}

	constexpr float dequantizeComponent(int8_t v){
		float const scale = 1 / 127.f;

		return v * scale;
	}

	template<typename T, typename F>
	constexpr void dequantizeF(VectorC<T> const &vector, F f){
		static_assert(checkT<T>(), "Only float and int8_t supported");

		#pragma GCC ivdep
		for(size_t i = 0; i < vector.size(); ++i)
			f(i, dequantizeComponent(vector[i]));
	}

	// ------------------------

	template<typename T>
	constexpr float denormalizeComponent(T v, float const magnitude){
		static_assert(checkT<T>(), "Only float and int8_t supported");

		return dequantizeComponent(v) * magnitude;
	}

	template<typename T>
	constexpr FVector const &denormalize(VectorC<T> const &vector, float const magnitude, FVector &out){
		static_assert(checkT<T>(), "Only float and int8_t supported");

		assert(vector.size() == out.size());

		#pragma GCC ivdep
		for(size_t i = 0; i < vector.size(); ++i)
			out[i] = denormalizeComponent(vector[i], magnitude);

		return out;
	}

	template<typename T, typename F>
	constexpr void denormalizeF(VectorC<T> const &vector, float const magnitude, F f){
		static_assert(checkT<T>(), "Only float and int8_t supported");

		#pragma GCC ivdep
		for(size_t i = 0; i < vector.size(); ++i)
			f(i, denormalizeComponent(vector[i], magnitude));
	}

	// ------------------------

	namespace distance_cosine_impl_{

		template<typename T1, typename T2>
		float dotProduct(VectorC<T1> const &a, VectorC<T2> const &b){
			static_assert(checkT<T1>(), "Only float and int8_t supported");
			static_assert(checkT<T2>(), "Only float and int8_t supported");

			float dot = 0.0;

			#pragma GCC ivdep
			for (size_t i = 0; i < a.size(); ++i){
				float const a_i = dequantizeComponent(a[i]);
				float const b_i = dequantizeComponent(b[i]);

				dot += a_i * b_i;
			}

			return dot;
		}

		template<typename T1, typename T2>
		float cosineSimilarity(VectorC<T1> const &a, VectorC<T2> const &b){
			static_assert(checkT<T1>(), "Only float and int8_t supported");
			static_assert(checkT<T2>(), "Only float and int8_t supported");

			auto const dot = dotProduct(a, b);

			// returns 0.0 - 1.0

			auto const result = (1 + dot) / 2;

			constexpr float ZERO = 1E-6f;

			return result > ZERO ? result : 0;
		}
	}

	template<typename T1, typename T2>
	float distanceCosine(VectorC<T1> const &a, VectorC<T2> const &b, float /* aM */, float /* bM */){
		static_assert(checkT<T1>(), "Only float and int8_t supported");
		static_assert(checkT<T2>(), "Only float and int8_t supported");

		using namespace distance_cosine_impl_;

		return 1 - cosineSimilarity(a, b);
	}

	template<typename T1, typename T2>
	float distanceEuclideanSquared(VectorC<T1> const &a, VectorC<T2> const &b, float aM, float bM){
		static_assert(checkT<T1>(), "Only float and int8_t supported");
		static_assert(checkT<T2>(), "Only float and int8_t supported");

		using namespace distance_cosine_impl_;

		auto const dot = dotProduct(a, b);

		// returns 0.0 - INF

		auto const result = aM * aM + bM * bM - 2 * aM * bM * dot;

		constexpr float ZERO = 1E-6f;

		return result > ZERO ? result : 0;
	}

	template<typename T1, typename T2>
	float distanceEuclidean(VectorC<T1> const &a, VectorC<T2> const &b, float aM, float bM){
		static_assert(checkT<T1>(), "Only float and int8_t supported");
		static_assert(checkT<T2>(), "Only float and int8_t supported");

		auto const result = distanceEuclideanSquared(a, b, aM, bM);

		return std::sqrt(result);
	}

	template<typename T1, typename T2>
	float distanceCanberra(VectorC<T1> const &a, VectorC<T2> const &b, float /* aM */, float /* bM */){
		static_assert(checkT<T1>(), "Only float and int8_t supported");
		static_assert(checkT<T2>(), "Only float and int8_t supported");

		float result = 0.0f;

		for (size_t i = 0; i < a.size(); ++i) {
			float const a_i = dequantizeComponent(a[i]);
			float const b_i = dequantizeComponent(b[i]);

			constexpr float ZERO = 1E-6f;

			if (float const den = std::abs(a_i) + std::abs(b_i); den > ZERO)
				result += std::abs(a_i - b_i) / den;
		}

		return result;
	}

	template<typename T1, typename T2>
	float distanceManhattan(VectorC<T1> const &a, VectorC<T2> const &b, float aM, float bM){
		static_assert(checkT<T1>(), "Only float and int8_t supported");
		static_assert(checkT<T2>(), "Only float and int8_t supported");

		float result = 0.0f;

		for (size_t i = 0; i < a.size(); ++i) {
			float const a_i = denormalizeComponent(a[i], aM);
			float const b_i = denormalizeComponent(b[i], bM);

			result += std::abs(a_i - b_i);
		}

		return result;
	}

	template<typename T2>
	float distanceManhattanPrepared(FVector const &a, VectorC<T2> const &b, float, float bM){
		static_assert(checkT<T2>(), "Only float and int8_t supported");

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

	FVector &randomProjection(FVectorC const &vector, FVector &result, size_t seed = 0){
		using namespace random_projection_impl_;

		for (size_t x = 0; x < result.size(); ++x){
			float sum = 0.0f;

			#pragma GCC ivdep
			for (size_t y = 0; y < vector.size(); ++y)
				sum += vector[y] * MD(x, y, seed);

			result[x] = sum;
		}

		return result;
	}

	FVector const &randomProjectionNormalize(FVectorC const &vector, FVector &result){
		randomProjection(vector, result);

		normalizeInline(result);

		return result;
	}

	template<typename T>
	T randomProjectionBit(FVectorC const &vector, size_t seed = 0){
		using namespace random_projection_impl_;

		constexpr auto bits = sizeof(T) * 8;

		T result = 0;

		for (size_t x = 0; x < bits; ++x){
			float sum = 0.0f;

			#pragma GCC ivdep
			for (size_t y = 0; y < vector.size(); ++y)
				sum += vector[y] * MD(x, y, seed);

			T const I = sum > 0 ? 1 : 0;

			result |= I << x;
		}

		return result;
	}

} // namspace MyVectors

#endif // MY_VECTORS_H_


