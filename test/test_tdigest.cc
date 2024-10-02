#include <iostream>

#include "tdigest.h"

constexpr size_t SIZE	= 100;

constexpr double delta	= 0.05;

int main(){
	RawTDigest td{ SIZE };

	auto *b = reinterpret_cast<RawTDigest::TDigest *>(malloc(td.bytes()));

	for(size_t i = 0; i < 100; ++i){
		td.add(b, delta, 10);

		assert(td.weight(b) == i + 1);
	}

	std::cout << td.size(b) << '\n';

	for(size_t i = 0; i < 5; ++i)
		td.add(b, delta, 10);

	std::cout << td.size(b) << '\n';

	td.add(b, delta,  -1);
	td.add(b, delta, 100);

	std::cout << td.percentile(b,  .00) << '\n';
	std::cout << td.percentile(b, 1.00) << '\n';
	std::cout << td.percentile(b,  .50) << '\n';
	std::cout << td.percentile(b,  .90) << '\n';
	std::cout << td.percentile(b,  .95) << '\n';

	std::cout << td.min(b) << '\n';
	std::cout << td.max(b) << '\n';

}

