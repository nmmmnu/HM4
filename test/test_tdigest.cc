#include <iostream>

#include "tdigest.h"

constexpr size_t SIZE	= 100;

constexpr double delta	= 0.05;

namespace{

	auto alloc(size_t size){
		auto *p = reinterpret_cast<RawTDigest::TDigest *>(malloc(size));

		memset(p, 0, size);

		return p;
	}

} // namespace

int main(){
	RawTDigest td{ SIZE };

	auto *a = alloc(td.bytes());
	auto *b = alloc(td.bytes());

	for(size_t i = 0; i < 100; ++i){
		td.add(a, delta, 10);
		td.add(b, delta, 10, 2);

		assert(td.weight(a) == 1*(i + 1));
		assert(td.weight(b) == 2*(i + 1));
	}

	std::cout << "size   A: " << td.size(a) << '\n';
	std::cout << "weight A: " << td.weight(a) << '\n';
	std::cout << "size   B: " << td.size(b) << '\n';
	std::cout << "weight B: " << td.weight(b) << '\n';

	for(size_t i = 0; i < 5; ++i){
		td.add(a, delta, 10);
		td.add(b, delta, 10, 2);
	}

	std::cout << "size   A after: " << td.size(a) << '\n';
	std::cout << "weight A after: " << td.weight(a) << '\n';
	std::cout << "size   B after: " << td.size(b) << '\n';
	std::cout << "weight B after: " << td.weight(b) << '\n';

	// -------------

	RawTDigest tdc{ SIZE * 2 };
	auto *c = alloc(tdc.bytes());

	merge(tdc, c, delta, td, a);

	std::cout << "size   C: " << td.size(c) << '\n';
	std::cout << "weight C: " << td.weight(c) << '\n';

	merge(tdc, c, delta, td, b);

	std::cout << "size   C end: " << td.size(c) << '\n';
	std::cout << "weight C end: " << td.weight(c) << '\n';

	// -------------

	td.clear(a);

	for(int i = 0; i < 1000; ++i)
		td.add(a, 0, i);

	std::cout << "trimmed mean 0.000 - 1.000: " << td.trimmedMean(a, 0.000, 1.000) << '\n';
	std::cout << "trimmed mean 0.000 - 1.000: " << td.mean(a) << '\n';
	std::cout << "trimmed mean 0.100 - 1.000: " << td.trimmedMean(a, 0.100, 1.000) << '\n';
	std::cout << "trimmed mean 0.100 - 0.900: " << td.trimmedMean(a, 0.100, 0.900) << '\n';
	std::cout << "trimmed mean 0.400 - 0.600: " << td.trimmedMean(a, 0.400, 0.600) << '\n';

	// -------------

	RawTDigest tdd{ 3 };

	auto *d = alloc(td.bytes());

	for(int i = 0; i < 1000; ++i)
		tdd.add(d, 0, i);

	tdd.print(d);

	std::cout << "trimmed mean 0.000 - 1.000: " << tdd.trimmedMean(d, 0.000, 1.000) << '\n';
	std::cout << "trimmed mean 0.000 - 1.000: " << tdd.mean(d) << '\n';
	std::cout << "trimmed mean 0.100 - 1.000: " << tdd.trimmedMean(d, 0.100, 1.000) << '\n';
	std::cout << "trimmed mean 0.100 - 0.900: " << tdd.trimmedMean(d, 0.100, 0.900) << '\n';
	std::cout << "trimmed mean 0.400 - 0.600: " << tdd.trimmedMean(d, 0.400, 0.600) << '\n';
	std::cout << "trimmed mean 0.499 - 0.501: " << tdd.trimmedMean(d, 0.499, 0.501) << '\n';


}

