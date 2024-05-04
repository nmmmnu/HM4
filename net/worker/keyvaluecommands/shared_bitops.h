#ifndef SHARED_BITOPS_H_
#define SHARED_BITOPS_H_

#include <cstdint>

namespace net::worker::shared::bit{

	struct BitOps{
		using size_t = std::size_t;

		size_t  const n_byte;
		uint8_t const n_mask;

		constexpr BitOps(size_t n) :
				n_byte(n / 8			),
				n_mask(uint8_t( 1 << (n % 8) )	){}

		constexpr size_t size() const{
			return n_byte + 1;
		}

		void set(char *buffer, bool bit) const{
			uint8_t *bits = reinterpret_cast<uint8_t *>(buffer);

			if (bit)
				bits[n_byte] |=  n_mask;
			else
				bits[n_byte] &= ~n_mask;
		}

		bool get(const char *buffer) const{
			const uint8_t *bits = reinterpret_cast<const uint8_t *>(buffer);

			return bits[n_byte] & n_mask;
		}

		constexpr static size_t max_bits(size_t buffer_size){
			// 4 bytes = 4 * 8 = 16 bits, 0 to 15.
			return buffer_size * 8;
		}

		constexpr static size_t size(size_t n){
			// 15 bits = 15 / 8 = 1 bytes + 1 correction, 0 to 15
			return n / 8 + 1;
		}
	};

}

#endif

