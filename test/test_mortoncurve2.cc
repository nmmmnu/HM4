#include "mortoncurve.h"

#include <cstdio>
#include <vector>
#include <algorithm>

namespace{

	template<typename IT>
	auto skipUntil(IT begin, IT const end, uint64_t zzz1, uint64_t zzz2){
		for(auto it = begin; it != end; ++it){
			auto &[zzz, x, y] = *it;

			//printf("%6lu %4lu %4lu (%lu %lu)\n", zzz, x, y, zzz1, zzz2);

			if (zzz >= zzz1 && zzz <= zzz2)
				return it;
		}

		return end;
	}

	template<bool BigMinOptimized, typename IT, size_t const max_out_of_range = 10>
	size_t search(	IT begin, IT const end,
			uint32_t const x_min, uint32_t const x_max,
			uint32_t const y_min, uint32_t const y_max
		){

		using namespace morton_curve;

		auto const zzz1 = toMorton2D32({x_min, y_min});
		auto const zzz2 = toMorton2D32({x_max, y_max});

		printf("zzz -> %6lu %6lu\n", zzz1, zzz2);

		size_t skips  = 1;
		auto it = skipUntil(begin, end, zzz1, zzz2);

		if (it == end){
			printf("null range\n");
			return 0;
		}

		size_t count = 0;
		size_t out_of_range = 0;

		while(it != end){
			auto const &[zzz, x, y] = *it;

			const char *s = "";

			if (zzz > zzz2){
				printf("Range end. Done!\n");
				goto out;
			}

			if (x >= x_min && x <= x_max && y >= y_min && y <= y_max){
				++count;
				out_of_range = 0;

				s = " <=== result";
			}else{
				if constexpr(BigMinOptimized){
					if (++out_of_range > max_out_of_range){

						printf("Values for BIGMIN: %lu %lu %lu\n", zzz, zzz1, zzz2);

						auto const bigmin = computeBigMinFromMorton2D32(zzz, zzz1, zzz2);

						printf("BIGMIN: %lu\n", bigmin);
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
				printf("%6lu %4u %4u %s\n", zzz, x, y, s);

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

	struct P{
		uint64_t zzz;
		uint32_t x;
		uint32_t y;

		constexpr P(uint64_t zzz, uint32_t x, uint32_t y) : zzz(zzz), x(x), y(y){}

		constexpr bool operator <(P const &other){
			return zzz < other.zzz;
		}
	};

	std::vector<P> container;

	constexpr size_t x_len = 100;
	constexpr size_t y_len = 100;

	container.reserve(x_len * y_len);

	// CONSTRUCT

	printf("Construct...\n");

	if constexpr(1){
		using namespace morton_curve;

		for(uint32_t y = 0; y < y_len; ++y)
			for(uint32_t x = 0; x < x_len; ++x)
				container.emplace_back(
					toMorton2D32({x, y}),
					x, y
				);

		std::sort(std::begin(container), std::end(container));
	}

	// PRINT
	if constexpr(0){
		using namespace morton_curve;

		for(auto const &[zzz, x, y] : container){
			printf("%6lu %4lu %4lu\n", zzz, x, y);
		}
	}

	// SEARCH

	printf("Search...\n");

	[[maybe_unused]]
	auto const count1 = search<1>(
			std::begin(container), std::end(container),
			x_min, x_max,
			y_min, y_max
	);

	// VERIFY

	printf("Verify...\n");

	[[maybe_unused]]
	auto const count2 = search<0>(
			std::begin(container), std::end(container),
			x_min, x_max,
			y_min, y_max
	);

	if (count1 == count2)
		printf("ALL OK! %zu Results\n", count1);
	else
		printf("ERROR! %zu vs %zu Results\n", count1, count2);
}


