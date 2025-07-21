#include "mortoncurve.h"

#include <cstdio>

#ifdef HAVE_UINT128_T

#include <vector>
#include <algorithm>

namespace{

	template<typename IT>
	auto skipUntil(IT begin, IT const end, uint128_t zzz1, uint128_t zzz2){
		for(auto it = begin; it != end; ++it){
			auto &[zzz, x, y, z, w] = *it;

			//printf("%6lu %4lu %4lu %4lu (%lu %lu)\n", zzz, x, y, z, zzz1, zzz2);

			if (zzz >= zzz1 && zzz <= zzz2)
				return it;
		}

		return end;
	}

	template<bool BigMinOptimized, typename IT, size_t const max_out_of_range = 10>
	size_t search(	IT begin, IT const end,
			uint32_t const x_min, uint32_t const x_max,
			uint32_t const y_min, uint32_t const y_max,
			uint32_t const z_min, uint32_t const z_max,
			uint32_t const w_min, uint32_t const w_max
		){

		using namespace morton_curve;

		auto const zzz1 = toMorton4D32({x_min, y_min, z_min, w_min});
		auto const zzz2 = toMorton4D32({x_max, y_max, z_max, w_max});

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
			auto const &[zzz, x, y, z, w] = *it;

			const char *s = "";

			if (zzz > zzz2){
				printf("Range end. Done!\n");
				goto out;
			}

			if (		x >= x_min && x <= x_max &&
					y >= y_min && y <= y_max &&
					z >= z_min && z <= z_max &&
					w >= w_min && w <= w_max
			){
				++count;
				out_of_range = 0;

				s = " <=== result";
			}else{
				if constexpr(BigMinOptimized){
					if (++out_of_range > max_out_of_range){

						printf("Values for BIGMIN: %lu %lu %lu\n", (uint64_t) zzz, (uint64_t) zzz1, (uint64_t) zzz2);

						auto const bigmin = computeBigMinFromMorton4D32(zzz, zzz1, zzz2);

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
				printf("%6lu %4u %4u %4u %4u %s\n", (uint64_t) zzz, x, y, z, w, s);

			++it;
		}

	out:
		printf("Count: %zu\n", count);

		return count;
	}

} // anonymous namespace

int main(){
	[[maybe_unused]] constexpr uint32_t x_min = 15;
	[[maybe_unused]] constexpr uint32_t x_max = 16;

	[[maybe_unused]] constexpr uint32_t y_min = 25;
	[[maybe_unused]] constexpr uint32_t y_max = 27;

	[[maybe_unused]] constexpr uint32_t z_min = 31;
	[[maybe_unused]] constexpr uint32_t z_max = 33;

	[[maybe_unused]] constexpr uint32_t w_min = 41;
	[[maybe_unused]] constexpr uint32_t w_max = 42;

	struct P{
		uint128_t zzz;
		uint32_t x;
		uint32_t y;
		uint32_t z;
		uint32_t w;

		constexpr P(uint128_t zzz, uint32_t x, uint32_t y, uint32_t z, uint32_t w) : zzz(zzz), x(x), y(y), z(z), w(w){}

		constexpr bool operator <(P const &other){
			return zzz < other.zzz;
		}
	};

	std::vector<P> container;

	constexpr size_t x_len = 50;
	constexpr size_t y_len = 50;
	constexpr size_t z_len = 50;
	constexpr size_t w_len = 50;

	container.reserve(x_len * y_len);

	// CONSTRUCT

	printf("Construct...\n");

	if constexpr(1){
		using namespace morton_curve;

		for(uint32_t w = 0; w < w_len; ++w)
			for(uint32_t z = 0; z < z_len; ++z)
				for(uint32_t y = 0; y < y_len; ++y)
					for(uint32_t x = 0; x < x_len; ++x)
						container.emplace_back(
							toMorton4D32({x, y, z, w}),
							x, y, z, w
						);


		std::sort(std::begin(container), std::end(container));
	}

	// PRINT
	if constexpr(0){
		using namespace morton_curve;

		for(auto const &[zzz, x, y, z, w] : container){
			printf("%6lu %4u %4u %4u %4u\n", (uint64_t)zzz, x, y, z, w);
		}
	}

	// SEARCH

	printf("Search...\n");

	[[maybe_unused]]
	auto const count1 = search<1>(
			std::begin(container), std::end(container),
			x_min, x_max,
			y_min, y_max,
			z_min, z_max,
			w_min, w_max
	);

	// VERIFY

	printf("Verify...\n");

	[[maybe_unused]]
	auto const count2 = search<0>(
			std::begin(container), std::end(container),
			x_min, x_max,
			y_min, y_max,
			z_min, z_max,
			w_min, w_max
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



