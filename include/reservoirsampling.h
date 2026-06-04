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
		template <std::size_t N>
		using size_type_ =
			std::conditional_t<N <= std::numeric_limits<std::uint8_t >::max() - 1,	std::uint8_t	,
			std::conditional_t<N <= std::numeric_limits<std::uint16_t>::max() - 1,	std::uint16_t	,
			std::conditional_t<N <= std::numeric_limits<std::uint32_t>::max() - 1,	std::uint32_t	,
												std::uint64_t
			> > >;

		template<size_t MaxItemSize>
		struct Item_{
			using size_type = size_type_<MaxItemSize>;

			size_type	size;			//  1, 2, 4, 8
			char		item[MaxItemSize];	// 63, 127...

			constexpr operator bool() const{
				return size;
			}

			constexpr auto getItem() const{
				return std::string_view{
						item,
						betoh(size)
				};
			}

			void setItem(std::string_view sv){
				assert(sv.size() <= MaxItemSize);

				size  = htobe(
						static_cast<size_type>(sv.size())
				);

				memcpy(item, sv.data(), sv.size());
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

			constexpr
			auto incrCount(){
				auto const count = getCount();
				this->count = htobe(count + 1);
				return count;
			}
		} __attribute__((__packed__));

	} // namespace reservoir_sampling_impl_

	template<size_t MaxItemSize>
	struct RawReservoirSampling{
		using List		= reservoir_sampling_impl_::List<MaxItemSize>;
		using Item		= typename List::Item;

		static_assert(std::is_trivial<List>::value, "List must be POD type");
		// static_assert( sizeof(List) % sizeof(int64_t) == 0 );

		enum class Added{
			INIT,
			YES,
			NO
		};

		// --------------------------

		explicit constexpr RawReservoirSampling(size_t size) : size_(size){}

		// --------------------------

		constexpr static bool isItemValid(std::string_view sv){
			return sv.size() >= 1 && sv.size() <= MaxItemSize;
		}

		// --------------------------

		constexpr size_t size() const{
			return size_;
		}

		constexpr size_t bytes() const{
			return sizeof(List) - sizeof(Item) + sizeof(Item) * size();
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
				rs.items[pos].setItem(item);
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

		std::string_view get(List const &rs, size_t index) const{
			return rs.items[index];
		}

	private:
		size_t	size_;
	};



	using RawReservoirSampling16   = RawReservoirSampling<  16 - 1>; //   2 x 8
	using RawReservoirSampling32   = RawReservoirSampling<  32 - 1>; //   4 x 8
	using RawReservoirSampling40   = RawReservoirSampling<  40 - 1>; //   5 x 8, IP6 is 8 sets x 4 chars = 32 chars, separated by a colon = 39 chars.
	using RawReservoirSampling64   = RawReservoirSampling<  64 - 1>; //   8 x 8
	using RawReservoirSampling128  = RawReservoirSampling< 128 - 1>; //  16 x 8
	using RawReservoirSampling256  = RawReservoirSampling< 256 - 1>; //  32 x 8
	using RawReservoirSampling512  = RawReservoirSampling< 512 - 1>; //  64 x 8
	using RawReservoirSampling1024 = RawReservoirSampling<1024 - 1>; // 128 x 8

} // namespace reservoir_sampling

#endif

