#ifndef MY_OPRIME_H_
#define MY_OPRIME_H_

#include <cstddef>
#include <cstdint>

namespace oprime{
	namespace oprime_impl_{
		constexpr uint8_t SMALL_PRIMES[] = {
			  3,   5,   7,  11,  13,  17,  19,  23,  29,  31,
			 37,  41,  43,  47,  53,  59,  61,  67,  71,  73,
			 79,  83,  89,  97, 101, 103, 107, 109, 113, 127,
			131, 137, 139, 149, 151, 157, 163, 167, 173, 179,
			181, 191, 193, 197, 199, 211, 223, 227, 229, 233,
			239, 241, 251
		};

		constexpr size_t SMALL_PRIMES_SIZE = sizeof(SMALL_PRIMES) / sizeof(SMALL_PRIMES[0]);

		constexpr bool isOPrime(size_t value){
			for (size_t i = 0; i < SMALL_PRIMES_SIZE; ++i)
				if (value % SMALL_PRIMES[i] == 0)
					return false;

			return true;
		}

		constexpr uint64_t PRIME_GAP =  1'425'172'824'437'699'411LLU;	// 1132 numbers
	}

	constexpr size_t next(size_t start){
		if (start <= 3)
			return 3;

		if (!(start & 1))
			++start;

		using namespace oprime_impl_;

		while(true){
			if (isOPrime(start))
				return start;

			start += 2;

			// The biggest prime gap known to men is PRIME_GAP
			// there are 1550 non prime numbers.
			// So we do not need to have break code here.
		}
	}

} // namespace oprime

#endif

