#ifndef misra_gries_H_
#define misra_gries_H_

#include <cstdint>
#include <cassert>
#include <cstring>
#include <string_view>
#include <limits>

#include "myendian.h"

namespace misra_gries{
	namespace misra_gries_impl_{
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

			uint64_t	score;			//  8
			size_type	size;			//  1, 2, 4, 8
			char		item[MaxItemSize];	// 63, 127...

			constexpr static uint64_t ZERO = htobe( uint64_t{0} );
			constexpr static uint64_t ONE  = htobe( uint64_t{1} );
			constexpr static uint64_t MAX  = htobe( std::numeric_limits<uint64_t>::max() );

			// -----

			constexpr operator bool() const{
				return score;
			}

			// -----

			void setScore(uint64_t score){
				this->score = htobe(score);
			}

			constexpr uint64_t getScore() const{
				return betoh(score);
			}

			// -----

			constexpr auto getItem() const{
				return std::string_view{
						item,
						betoh(size)
				};
			}

			constexpr bool cmpItem(std::string_view s) const{
				return s == getItem();
			}

			uint64_t setItem(std::string_view sv){
				assert(sv.size() <= MaxItemSize);

				score = ONE;

				size  = htobe(
						static_cast<size_type>(sv.size())
				);

				memcpy(item, sv.data(), sv.size());

				return 1;
			}

			// -----

			uint64_t incr(){
				switch(score){
				case MAX:
					return std::numeric_limits<uint64_t>::max();

				default:
					auto const s = getScore() + 1;
					setScore(s);
					return s;
				}
			}

			void decr(){
				switch(score){
				case ZERO:
					return;

				case ONE:
					score = 0;
					return;

				default:
					auto const s = getScore();
					return setScore(s - 1);
				}
			}

		} __attribute__((__packed__));

		template<size_t MaxItemSize>
		struct List{
			using Item		= Item_<MaxItemSize>;

			Item			items[1];
		} __attribute__((__packed__));

	} // namespace misra_gries_impl_

	template<size_t MaxItemSize>
	struct RawMisraGries{
		using List		= misra_gries_impl_::List<MaxItemSize>;
		using Item		= typename List::Item;

		static_assert(std::is_trivial<List>::value, "List must be POD type");
		// sstatic_assert( sizeof(List) % sizeof(int64_t) == 0 );

		// --------------------------

		constexpr RawMisraGries(size_t size) : size_(size){
			assert(size != NOT_FOUND);
		}

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

		void clear(List *mg) const{
			memset(mg, 0, bytes());
		}

		void load(List *mg, const void *src) const{
			memcpy(mg, src, bytes());
		}

		void store(const List *mg, void *dest) const{
			memcpy(dest, mg, bytes());
		}

		// --------------------------

		uint64_t add(List &mg, std::string_view item) const{
			size_t indexEmptySlot = NOT_FOUND;

			for(size_t i = 0; i < size(); ++i){
				auto &x = mg.items[i];

				if (!x){
					indexEmptySlot = i;
					continue;
				}

				if (x.cmpItem(item))
					return x.incr();
			}

			// if we are here, item is not found in the array

			// insert into empty slot
			if (indexEmptySlot != NOT_FOUND){
				auto &x = mg.items[indexEmptySlot];
				return x.setItem(item);
			}

			for(size_t i = 0; i < size(); ++i){
				auto &x = mg.items[i];
				x.decr();
			}

			return 0;
		}

	private:
		constexpr static size_t NOT_FOUND = std::numeric_limits<size_t>::max();

		size_t	size_;
	};



	using RawMisraGries16   = RawMisraGries<  16 - 1>; //   2 x 8
	using RawMisraGries32   = RawMisraGries<  32 - 1>; //   4 x 8
	using RawMisraGries40   = RawMisraGries<  40 - 1>; //   5 x 8, IP6 is 8 sets x 4 chars = 32 chars, separated by a colon = 39 chars.
	using RawMisraGries64   = RawMisraGries<  64 - 1>; //   8 x 8
	using RawMisraGries128  = RawMisraGries< 128 - 1>; //  16 x 8
	using RawMisraGries256  = RawMisraGries< 256 - 1>; //  32 x 8
	using RawMisraGries512  = RawMisraGries< 512 - 1>; //  64 x 8
	using RawMisraGries1024 = RawMisraGries<1024 - 1>; // 128 x 8

} // namespace misra_gries

#endif

