#include "mortoncurve.h"

#include <vector>
#include <algorithm>
#include <cstdio>

namespace{

	template<typename IT>
	auto skipUntil(IT begin, IT const end, uint64_t z1, uint64_t z2){
		for(auto it = begin; it != end; ++it)
			if (it->z >= z1 && it->z <= z2)
				return it;

		return end;
	}

	template<typename IT, size_t const max_out_of_range = 10>
	size_t search(	IT begin, IT const end,
			uint32_t const x_min, uint32_t const x_max,
			uint32_t const y_min, uint32_t const y_max
		){

		using namespace morton_curve;

		auto const target1 = toMorton2D(x_min, y_min);
		auto const target2 = toMorton2D(x_max, y_max);

		size_t skips  = 1;
		auto it = skipUntil(begin, end, target1, target2);

		if (it == end)
			return 0;

		size_t count = 0;
		size_t out_of_range = 0;

		while(it != end){
			auto const &[z, x, y] = *it;

			const char *s;

			if (z > target2){
				printf("Range end. Done!\n");
				return count;
			}

			if (x >= x_min && x <= x_max && y >= y_min && y <= y_max){
				++count;
				out_of_range = 0;
				s = "inside the range";
			}else{
				++out_of_range;
				s = "outside of the range";

				if (out_of_range > max_out_of_range){
					// Huston we have a problem!!!

					auto const bigmin = computeBigMinFromMorton2D(z, target1, target2);

					printf("BIGMIN: %lu\n", bigmin);
					printf("Skips : %zu\n", skips);

					if (bigmin <= z)
						return count;

					++skips;
					it = skipUntil(begin, end, bigmin, target2);

					if (it == end)
						return count;

					out_of_range = 0;

					continue;
				}
			}

			printf("%5lu %3lu %3lu %-25s %zu\n", z, x, y, s, out_of_range);

			++it;
		}

		return count;
	}

	template<typename IT>
	size_t search_iterative(IT begin, IT end, uint32_t const x_min, uint32_t const x_max, uint32_t const y_min, uint32_t const y_max){
		size_t count = 0;

		for(auto it = begin; it != end; ++it){
			auto const &[z, x, y] = *it;

			if (x >= x_min && x <= x_max && y >= y_min && y <= y_max)
				++count;
		}

		return count;
	}

} // anonymous namespace

int main(){
	constexpr uint32_t x_min = 55;
	constexpr uint32_t x_max = 66;

	constexpr uint32_t y_min = 55;
	constexpr uint32_t y_max = 66;

	struct P{
		uint64_t z;
		uint64_t x;
		uint64_t y;

		constexpr P(uint64_t z, uint64_t x, uint64_t y) : z(z), x(x), y(y){}

		constexpr bool operator <(P const &other){
			return z < other.z;
		}
	};

	std::vector<P> container;

	constexpr size_t x_len = 16 * 1024;
	constexpr size_t y_len = 16 * 1024;

	container.reserve(x_len * y_len);

	// CONSTRUCT

	printf("Construct...\n");

	if constexpr(1){
		using namespace morton_curve;

		auto const target1 = toMorton2D(x_min, y_min);
		auto const target2 = toMorton2D(x_max, y_max);

		for(uint32_t y = 0; y < y_len; ++y){
			constexpr bool print = 0;

			for(uint32_t x = 0; x < x_len; ++x){
				auto const z = toMorton2D(x, y);

				container.emplace_back(z, x, y);

				if constexpr(print){
					if (x >= x_min && x <= x_max && y >= y_min && y <= y_max)
						printf("[%3lu]", z);
					else
					if (z >= target1 && z <= target2)
						printf("-%3lu-", z);
					else
						printf(" %3lu ", z);

					printf(" ");
				}
			}

			if constexpr(print){
				printf("\n");
			}
		}
		std::sort(std::begin(container), std::end(container));
	}

	// PRINT
	if constexpr(0){
		using namespace morton_curve;

		auto const target1 = toMorton2D(x_min, y_min);
		auto const target2 = toMorton2D(x_max, y_max);

		for(auto const &[z, x, y] : container){
			const char *s;

			if (x >= x_min && x <= x_max && y >= y_min && y <= y_max)
				s = "<= OK";
			else
			if (z >= target1 && z <= target2)
				s = "-----";
			else
				s = "";

			printf("%5lu %3lu %3lu %s\n", z, x, y, s);
		}
	}

	// SEARCH

	printf("Search...\n");

	auto const count1 = search(
			std::begin(container), std::end(container),
			x_min, x_max,
			y_min, y_max
	);

	// VERIFY

	printf("Verify...\n");

	auto const count2 = search_iterative(
			std::begin(container), std::end(container),
			x_min, x_max,
			y_min, y_max
	);

	if (count1 == count2)
		printf("ALL OK! %zu Results\n", count1);
}


