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
		template<typename SizeType, SizeType MaxItemSize>
		class Item_{
			using size_type = SizeType;

			uint64_t	score;			//  8
			size_type	size;			//  1, 2, 4, 8
			char		item[MaxItemSize];	// 63, 127...

			constexpr static uint64_t ZERO = htobe( uint64_t{0} );
			constexpr static uint64_t ONE  = htobe( uint64_t{1} );
			constexpr static uint64_t MAX  = htobe( std::numeric_limits<uint64_t>::max() );

		public:
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
				if (auto const s = betoh(size); s <= MaxItemSize)
					return std::string_view{ item, s };
				else
					return std::string_view{};
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

		template<typename SizeType, SizeType MaxItemSize>
		struct List{
			using Item	= Item_<SizeType,MaxItemSize>;

			Item		items[1];
		} __attribute__((__packed__));

	} // namespace misra_gries_impl_

	template<typename SizeType, SizeType MaxItemSize>
	struct RawMisraGries{
		using List		= misra_gries_impl_::List<SizeType,MaxItemSize>;
		using Item		= typename List::Item;

		static_assert(MaxItemSize >= 1,				"Size must be equalo ro greater than 1"	);
		static_assert(std::is_trivial<List>::value,		"List must be POD type"			);
		static_assert(sizeof(List) % sizeof(int64_t) == 0,	"List has to be multiple of 8"		);

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



	template<typename T, auto N>
	using RawMisraGries__   = RawMisraGries<T, N - sizeof(T)>;

	using RawMisraGries16   = RawMisraGries__< uint8_t,   16>; //   2 x 8
	using RawMisraGries32   = RawMisraGries__< uint8_t,   32>; //   4 x 8
	using RawMisraGries40   = RawMisraGries__< uint8_t,   40>; //   5 x 8, IP6 is 8 sets x 4 chars = 32 chars, separated by a colon = 39 chars.
	using RawMisraGries64   = RawMisraGries__< uint8_t,   64>; //   8 x 8
	using RawMisraGries128  = RawMisraGries__< uint8_t,  128>; //  16 x 8
	using RawMisraGries256  = RawMisraGries__< uint8_t,  256>; //  32 x 8
	using RawMisraGries512  = RawMisraGries__<uint16_t,  512>; //  64 x 8
	using RawMisraGries1024 = RawMisraGries__<uint16_t, 1024>; // 128 x 8

} // namespace misra_gries

#endif

