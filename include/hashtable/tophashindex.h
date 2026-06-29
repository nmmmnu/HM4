#ifndef MY_HASHTABLE_TOPHASH_INDEX_H_
#define MY_HASHTABLE_TOPHASH_INDEX_H_

#include <array>

#include <cstddef>
#include <cstdint>

namespace myhashtable{

	template<typename K, size_t Size>
	struct TopHashIndex{
		using token_type = uint8_t;

		constexpr static int NEXT	= 0;
		constexpr static int EMPTY	= 1;
		constexpr static int FOUND	= 2;



		constexpr static token_type getToken(uint64_t hash, K const &){
			return static_cast<token_type>(hash) | BIT;
		}

		constexpr int check(size_t ix, token_type token) const{
			if (data_[ix] == token)
				return FOUND;

			if (!data_[ix])
				return EMPTY;

			return NEXT;
		}

		constexpr void emplace(size_t ix, token_type token){
			data_[ix] = token;
		}

	private:
		constexpr static token_type BIT = 0x1;

		std::array<token_type, Size> data_;
	};

} // namespace myhashtable

#endif

