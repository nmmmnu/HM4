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
		template<size_t MaxItemSize>
		struct Item{
			static_assert(MaxItemSize <= std::numeric_limits<uint8_t>::max() );

			uint64_t	score;			//  8
			uint8_t		size;			//  1
			char		item[MaxItemSize];	// 63, 127...

			constexpr static uint64_t ZERO = htobe( uint64_t{0} );
			constexpr static uint64_t ONE  = htobe( uint64_t{1} );
			constexpr static uint64_t MAX  = htobe( std::numeric_limits<uint64_t>::max() );

			// -----

			constexpr static bool isItemValid(std::string_view item){
				return item.size() >= 1 && item.size() <= MaxItemSize;
			}

			// -----

			constexpr bool valid() const{
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
						size
				};
			}

			uint64_t setItem(std::string_view item){
				assert(item.size() <= MaxItemSize);

				score = ONE;

				size  = static_cast<uint8_t>(item.size());

				memcpy(this->item, item.data(), item.size());

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

	} // namespace misra_gries_impl_

	template<size_t MaxItemSize>
	struct RawMisraGries{
		using Item		= misra_gries_impl_::Item<MaxItemSize>;

		static_assert( sizeof(Item) == sizeof(int64_t) + MaxItemSize + 1 );
		static_assert( sizeof(Item[10]) % sizeof(int64_t) == 0 );

		// --------------------------

		constexpr RawMisraGries(size_t size) : size_(size){
			assert(size != NOT_FOUND);
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
			return sizeof(Item) * size();
		}

		// --------------------------

		void clear(Item *M) const{
			memset(M, 0, bytes());
		}

		void load(Item *M, const void *src) const{
			memcpy(M, src, bytes());
		}

		void store(const Item *M, void *dest) const{
			memcpy(dest, M, bytes());
		}

		// --------------------------

		uint64_t add(Item *M, std::string_view item) const{
			size_t indexEmptySlot = NOT_FOUND;

			for(size_t i = 0; i < size(); ++i){
				auto &x = M[i];

				if (!x.valid()){
					indexEmptySlot = i;
					continue;
				}

				if (item == x.getItem())
					return x.incr();
			}

			// if we are here, item is not found in the array

			// insert into empty slot
			if (indexEmptySlot != NOT_FOUND){
				auto &x = M[indexEmptySlot];
				return x.setItem(item);
			}

			for(size_t i = 0; i < size(); ++i){
				auto &x = M[i];
				x.decr();
			}

			return 0;
		}

	private:
		constexpr static size_t NOT_FOUND = std::numeric_limits<size_t>::max();

		size_t	size_;
	};



	using RawMisraGries16  = RawMisraGries< 16 - 1>; //  2 x 8
	using RawMisraGries32  = RawMisraGries< 32 - 1>; //  4 x 8
	using RawMisraGries40  = RawMisraGries< 40 - 1>; //  5 x 8, IP6 is 8 sets x 4 chars = 32 chars, separated by a colon = 39 chars.
	using RawMisraGries64  = RawMisraGries< 64 - 1>; //  8 x 8
	using RawMisraGries128 = RawMisraGries<128 - 1>; // 16 x 8
	using RawMisraGries256 = RawMisraGries<256 - 1>; // 32 x 8

} // namespace misra_gries

#endif

