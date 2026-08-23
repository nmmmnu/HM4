#ifndef MINHASH_RAW_H_
#define MINHASH_RAW_H_

#include <string_view>
#include <limits>
#include <algorithm>	// std::all_of

#include <cstdint>
#include <cstring>
#include <cassert>

#include "murmur_hash_64a.h"
#include "myendian.h"

namespace minhash{

	template<size_t Bits, typename MHT>
	class MinHash{
		constexpr static size_t Size = 1 << Bits;

		static_assert(Bits <= 16);

		static_assert(
			std::is_same_v<MHT, uint8_t > ||
			std::is_same_v<MHT, uint16_t> ||
			std::is_same_v<MHT, uint32_t>
		);

	public:
		using type = MHT;

	public:
		constexpr static size_t size(){
			return Size;
		}

		constexpr static size_t bytes(){
			return sizeof(MHT) * size();
		}

		constexpr static size_t bandCount(size_t bandSize){
			return bytes() / bandSize;
		}

		constexpr static size_t maxBandCount(){
			return bandCount(1);
		}

	public:
		static void clear(MHT *table) {
			memset(table, 0, bytes());
		}

		static void load(MHT *table, const void *src){
			memcpy(table, src, bytes());
		}

		static void store(const MHT *table, void *dest){
			memcpy(dest, table, bytes());
		}

	public:
		constexpr static bool add(MHT *table, std::string_view s){
			uint64_t const hash = murmur_hash64a(s);

			auto const mix = static_cast<uint16_t	>(hash >>  0);
			auto const val = static_cast<MHT	>(hash >> 32);

			auto const id  = mix & (Size - 1);

			// max instead of min
			if (betoh(table[id]) >= val)
				return false;

			table[id] = htobe(val);
			return true;
		}

	//	template<typename IT>
	//	constexpr static bool add(MHT *table, IT begin, IT end){
	//		bool result = false;
	//
	//		for(auto it = begin; it != end; ++it)
	//			result |= add(table, *it);
	//
	//		return result;
	//	}

		constexpr static void merge(MHT *result, const MHT *table){
			for (size_t i = 0; i < size(); ++i)
				if (betoh(result[i]) < betoh(table[i]))
					result[i] = table[i];
		}

		[[nodiscard]]
		constexpr static double jaccard(const MHT *a, const MHT *b){
			// J = (a ^ b) / (a u b)

			size_t matches = 0;
			size_t valid   = 0;

			for (size_t i = 0; i < size(); ++i){
				if (!a[i] && !b[i])
					continue;

				++valid;

				if (a[i] == b[i])
					++matches;
			}

			if (valid == 0)
				return 0;

			return static_cast<double>(matches) / static_cast<double>(valid);
		}

		[[nodiscard]]
		constexpr static double overlap(const MHT *a, const MHT *b){
			// O = (a ^ b) / min(a, b)

			size_t matches = 0;
			size_t validA  = 0;
			size_t validB  = 0;

			for (size_t i = 0; i < size(); ++i){
				if (!a[i] && !b[i])
					continue;

				if (a[i])
					++validA;

				if (b[i])
					++validB;

				if (a[i] == b[i])
					++matches;
			}

			auto const valid = std::min(validA, validB);

			if (valid == 0)
				return 0;

			return static_cast<double>(matches) / static_cast<double>(valid);
		}

		template<typename F>
		void bands(const MHT *table, size_t bandSize, F f){
			static_assert(bytes() <= std::numeric_limits<uint16_t>::max());

			assert(bandSize > 0 || bytes() % bandSize != 0);

			const auto *data = reinterpret_cast<const char *>(table);

			for (uint16_t i = 0; i < bandCount(bandSize); ++i){
				auto begin = data + i * bandSize;
				auto end   = data + i * bandSize + bandSize;

				if (std::all_of(begin, end, [](auto const b){ return b == 0; })){
				//	container.push_back();
					continue;
				}

				f(i, murmur_hash64a(begin, bandSize));
			}
		}
	};

	constexpr size_t DefaultBits = 12;

	using MinHash8  = MinHash<DefaultBits, uint8_t	>;
	using MinHash16 = MinHash<DefaultBits, uint16_t	>;
	using MinHash32 = MinHash<DefaultBits, uint32_t	>;

} // namespace minhash

#endif

