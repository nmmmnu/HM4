#include "mortoncurve.h"

#ifdef HAVE_UINT128_T

#include <vector>
#include <algorithm>
#include <cstdio>
#include <random>

namespace{

	template<typename IT>
	auto skipUntil(IT begin, IT const end, uint128_t zzz1, uint128_t zzz2){
		for(auto it = begin; it != end; ++it){
			auto &[zzz, v] = *it;

			//printf("%6lu %4lu %4lu %4lu (%lu %lu)\n", zzz, x, y, z, zzz1, zzz2);

			if (zzz >= zzz1 && zzz <= zzz2)
				return it;
		}

		return end;
	}

	template<size_t D, typename E>
	bool insideV(	std::array<E, D> const &target,
			std::array<E, D> const &v_min,
			std::array<E, D> const &v_max ){
		for(size_t i = 0; i < D; ++i)
			if (target[i] < v_min[i] || target[i] > v_max[i])
				return false;

		return true;
	}

	template<bool BigMinOptimized, typename IT, size_t const max_out_of_range = 100>
	size_t search(	IT begin, IT const end,
			std::array<uint8_t, 16> const v_min,
			std::array<uint8_t, 16> const v_max
		){

		using namespace morton_curve;

		auto const zzz1 = toMorton16D8(v_min);
		auto const zzz2 = toMorton16D8(v_max);

		printf("zzz -> %6lu %6lu\n", (uint64_t) zzz1, (uint64_t) zzz2);

		size_t skips  = 1;
		auto it = skipUntil(begin, end, zzz1, zzz2);

		if (it == end){
			printf("null range\n");
			return 0;
		}

		size_t count = 0;
		size_t out_of_range = 0;

		while(it != end){
			auto const &[zzz, v] = *it;

			const char *s = "";

			if (zzz > zzz2){
				printf("Range end. Done!\n");
				goto out;
			}

			if (insideV(v, v_min, v_max)){
				++count;
				out_of_range = 0;

				s = " <=== result";
			}else{
				if constexpr(BigMinOptimized){
					if (++out_of_range > max_out_of_range){

						printf("Values for BIGMIN: %lu %lu %lu\n", (uint64_t) zzz, (uint64_t) zzz1, (uint64_t) zzz2);

						auto const bigmin = computeBigMinFromMorton16D8(zzz, zzz1, zzz2);

						printf("BIGMIN: %lu\n", (uint64_t) bigmin);
						printf("Skips : %zu\n", skips);

						if (bigmin <= zzz)
							goto out;

						++skips;

						it = skipUntil(it, end, bigmin, zzz2);

						if (it == end)
							goto out;

						continue;
					}
				}
			}

			if constexpr(BigMinOptimized)
				printf("%6lu"
						"[ %u %u %u %u ]"
						"[ %u %u %u %u ]"
						"[ %u %u %u %u ]"
						"[ %u %u %u %u ] %s\n",
							(uint64_t) zzz,
							v[ 0], v[ 1], v[ 2], v[ 3],
							v[ 4], v[ 5], v[ 6], v[ 7],
							v[ 8], v[ 9], v[10], v[11],
							v[12], v[13], v[14], v[15],
							s
				);

			++it;
		}

	out:
		printf("Count: %zu\n", count);
		printf("Skips: %zu\n", skips);
		printf("Data Size: %zu\n", std::distance(begin, end));

		return count;
	}

} // anonymous namespace

int main(){
//	[[maybe_unused]] constexpr std::array<uint8_t, 16> v_min{ 0,1,0,1,  0,1,0,1,  0,1,0,1,  0,1,0,1 };
//	[[maybe_unused]] constexpr std::array<uint8_t, 16> v_max{ 1,2,1,2,  1,2,1,2,  1,2,1,2,  1,2,1,2 };

	[[maybe_unused]] constexpr std::array<uint8_t, 16> v_min{ 0,1,0,1,  0,1,0,1,  0,0,0,0,  0,0,0,0 };
	[[maybe_unused]] constexpr std::array<uint8_t, 16> v_max{ 1,2,1,2,  1,2,1,2,  0,0,0,0,  0,0,0,0 };

	struct P{
		uint128_t zzz;
		std::array<uint8_t, 16> v;

		constexpr P(uint128_t zzz, std::array<uint8_t, 16> const &v) : zzz(zzz), v(v){}

		constexpr bool operator <(P const &other){
			return zzz < other.zzz;
		}
	};

	std::vector<P> container;

	constexpr size_t msize16[] = {
			         0,
			         1,
			     65536, // 2 ^ 16
			  43046721, // 3 ^ 16
			4294967296, // 4 ^ 16
	};

	constexpr size_t len    = 3;
	constexpr size_t msize  = msize16[len];

	//container.reserve();

	// CONSTRUCT

	printf("Construct...\n");

	if constexpr(1){
		using namespace morton_curve;

		std::mt19937 gen(0);
		std::uniform_int_distribution<> dist(0, 100);

		size_t count = 0;

		for(uint8_t v00 = 0; v00 < len; ++v00)
		for(uint8_t v01 = 0; v01 < len; ++v01)
		for(uint8_t v02 = 0; v02 < len; ++v02)
		for(uint8_t v03 = 0; v03 < len; ++v03)
		for(uint8_t v04 = 0; v04 < len; ++v04)
		for(uint8_t v05 = 0; v05 < len; ++v05)
		for(uint8_t v06 = 0; v06 < len; ++v06)
		for(uint8_t v07 = 0; v07 < len; ++v07)
		for(uint8_t v08 = 0; v08 < len; ++v08)
		for(uint8_t v09 = 0; v09 < len; ++v09)
		for(uint8_t v10 = 0; v10 < len; ++v10)
		for(uint8_t v11 = 0; v11 < len; ++v11)
		for(uint8_t v12 = 0; v12 < len; ++v12)
		for(uint8_t v13 = 0; v13 < len; ++v13)
		for(uint8_t v14 = 0; v14 < len; ++v14)
		for(uint8_t v15 = 0; v15 < len; ++v15){
			if (++count % 100'000'000u == 0)
				printf("Count %10zu from %10zu, %10zu records...\n", count, msize, container.size());

			if (dist(gen) > 10)
				continue;

			std::array<uint8_t, 16> const v{
					v00, v01, v02, v03,
					v04, v05, v06, v07,
					v08, v09, v10, v11,
					v12, v13, v14, v15
			};

			container.emplace_back(
				toMorton16D8(v),
				v
			);
		}

		printf("Sorting %zu records\n", container.size());

		std::sort(std::begin(container), std::end(container));

		printf("Ready %zu records\n", container.size());
	}

	// PRINT
	if constexpr(0){
		using namespace morton_curve;

		// for(auto const &[zzz, x, y, z, w] : container){
		// 	printf("%6lu %4lu %4lu %4lu %4lu\n", zzz, x, y, z, w);
		// }
	}

	// SEARCH

	printf("Search...\n");

	[[maybe_unused]]
	auto const count1 = search<1>(
			std::begin(container), std::end(container),
			v_min, v_max
	);

	// VERIFY

	printf("Verify...\n");

	[[maybe_unused]]
	auto const count2 = search<0>(
			std::begin(container), std::end(container),
			v_min, v_max
	);

	if (count1 == count2)
		printf("ALL OK! %zu Results\n", count1);
	else
		printf("ERROR! %zu vs %zu Results\n", count1, count2);
}


#else

int main(){
	printf("No support for uint128_t\n");
}

#endif



