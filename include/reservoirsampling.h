#ifndef reservoir_sampling_H_
#define reservoir_sampling_H_

#include <cstdint>
#include <cassert>
#include <cstring>
#include <string_view>
#include <limits>
#include <random>	// mt19937_64, uniform_int_distribution
#include <type_traits>

#include "myendian.h"

namespace reservoir_sampling{
	namespace reservoir_sampling_impl_{
		template<size_t MaxItemSize>
		struct Item_{
			static_assert(MaxItemSize <= std::numeric_limits<uint8_t>::max() );

			uint8_t		size;			//  1
			char		item[MaxItemSize];	// 63, 127...

			constexpr static bool isItemValid(std::string_view item){
				return item.size() >= 1 && item.size() <= MaxItemSize;
			}

			constexpr operator bool() const{
				return size;
			}

			constexpr std::string_view getItem() const{
				return { item, size };
			}

		//	constexpr bool cmp(std::string_view s) const{
		//		return s == operator std::string_view();
		//	}

			Item_ &operator=(std::string_view item){
				assert(item.size() <= MaxItemSize);

				size  = static_cast<uint8_t>(item.size());

				memcpy(this->item, item.data(), item.size());

				return *this;
			}
		} __attribute__((__packed__));

		template<size_t MaxItemSize>
		struct List{
			using Item		= Item_<MaxItemSize>;

			uint64_t		count;
			Item			items[1];

			constexpr auto getCount() const{
				return betoh(count);
			}

		//	constexpr
		//	void setCount(uint64_t count){
		//		this->count = htobe(count);
		//	}

			constexpr
			auto incrCount(){
				auto const count = getCount();
				this->count = htobe(count + 1);
				return count;
			}
		};
	} // namespace reservoir_sampling_impl_

	template<size_t MaxItemSize>
	struct RawReservoirSampling{
		using List		= reservoir_sampling_impl_::List<MaxItemSize>;
		using Item		= typename List::Item;

		static_assert(std::is_trivial<List>::value, "List must be POD type");

		enum class Added{
			INIT,
			YES,
			NO
		};

		// check if structs are packed
		static_assert( sizeof(Item) == MaxItemSize + 1 );
		static_assert( sizeof(Item[10]) % sizeof(int64_t) == 0 );
		static_assert( sizeof(List) == sizeof(int64_t) + sizeof(Item) );

		// --------------------------

		explicit constexpr RawReservoirSampling(size_t size) : size_(size){
		}

		// --------------------------

		constexpr static bool isItemValid(std::string_view item){
			return Item::isItemValid(item);
		}

		// --------------------------

		constexpr size_t size() const{
			return size_;
		}

		constexpr size_t bytes() const{
			return sizeof(int64_t) + sizeof(Item) * size();
		}

		// --------------------------

		void clear(List *rs) const{
			memset(rs, 0, bytes());
		}

		void load(List *rs, const void *src) const{
			memcpy(rs, src, bytes());
		}

		void store(const List *rs, void *dest) const{
			memcpy(dest, rs, bytes());
		}

		// --------------------------

		Added add(List &rs, std::string_view item, std::mt19937_64 &gen64) const{
			auto put = [&rs, item](size_t pos, Added status){
				rs.items[pos] = item;
				return status;
			};

			auto const count = rs.incrCount();

			if (count < size())
				return put(count, Added::INIT);

			// yep, thats fast
			std::uniform_int_distribution<uint64_t> dis(0, count);

			if (auto i = dis(gen64); i < size())
				return put(i, Added::YES);

			return Added::NO;
		}

		Added add(List *rs, std::string_view item, std::mt19937_64 &gen64) const{
			return add(*rs, item, gen64);
		}

		std::string_view get(List const &rs, size_t index) const{
			return rs.items[index];
		}

		std::string_view get(const List *rs, size_t index) const{
			return get(*rs, index);
		}

	private:
		size_t	size_;
	};



	using RawReservoirSampling16  = RawReservoirSampling< 16 - 1>; //  2 x 8
	using RawReservoirSampling32  = RawReservoirSampling< 32 - 1>; //  4 x 8
	using RawReservoirSampling40  = RawReservoirSampling< 40 - 1>; //  5 x 8, IP6 is 8 sets x 4 chars = 32 chars, separated by a colon = 39 chars.
	using RawReservoirSampling64  = RawReservoirSampling< 64 - 1>; //  8 x 8
	using RawReservoirSampling128 = RawReservoirSampling<128 - 1>; // 16 x 8
	using RawReservoirSampling256 = RawReservoirSampling<256 - 1>; // 32 x 8

} // namespace reservoir_sampling

#endif

