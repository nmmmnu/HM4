#ifndef HEAVY_HITTER_H_
#define HEAVY_HITTER_H_

#include <cstdint>
#include <cassert>
#include <cstring>
#include <string_view>
#include <limits>

#include "myendian.h"

namespace heavy_hitter{
	namespace heavy_hitter_impl_{
		template<bool>
		struct Comparator;

		template<>
		struct Comparator<true>{
			constexpr static int64_t start = std::numeric_limits<int64_t>::max();

			constexpr bool operator()(int64_t a, int64_t b) const{
				return a > b;
			}
		};

		template<>
		struct Comparator<false>{
			constexpr static int64_t start = std::numeric_limits<int64_t>::min();

			constexpr bool operator()(int64_t a, int64_t b) const{
				return a < b;
			}
		};

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

			// -----

			constexpr operator bool() const{
				return size;
			}

			// -----

			constexpr int64_t getScore() const{
				return static_cast<int64_t>(
					betoh(score)
				);
			}

			void setScore(int64_t score){
				this->score = htobe(
					static_cast<uint64_t>(score)
				);
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

			void setItem(std::string_view sv){
				assert(sv.size() <= MaxItemSize);

				size  = htobe(
						static_cast<size_type>(sv.size())
				);

				memcpy(item, sv.data(), sv.size());
			}

			// -----

			void set(int64_t score){
				setScore(score);
			}

			void set(std::string_view item){
				setItem(item);
			}

			void set(int64_t score, std::string_view item){
				set(score);
				set(item);
			}
		} __attribute__((__packed__));

		template<size_t MaxItemSize>
		struct List{
			using Item		= Item_<MaxItemSize>;

			Item			items[1];
		} __attribute__((__packed__));

	} // namespace heavy_hitter_impl_

	template<size_t MaxItemSize>
	struct RawHeavyHitter{
		template<bool Up>
		using Comparator	= heavy_hitter_impl_::Comparator<Up>;
		using List		= heavy_hitter_impl_::List<MaxItemSize>;
		using Item		= typename List::Item;

		static_assert(std::is_trivial<List>::value, "List must be POD type");
		// static_assert( sizeof(List) % sizeof(int64_t) == 0 );

		// --------------------------

		constexpr RawHeavyHitter(size_t size) : size_(size){
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

		void clear(List *hh) const{
			memset(hh, 0, bytes());
		}

		void load(List *hh, const void *src) const{
			memcpy(hh, src, bytes());
		}

		void store(const List *hh, void *dest) const{
			memcpy(dest, hh, bytes());
		}

		// --------------------------

		template<bool Up>
		bool add(List &hh, std::string_view item, int64_t score) const{
			Comparator<Up> const comp;

			int64_t  minScore       = comp.start;

			size_t   indexMinScore  = NOT_FOUND;
			size_t   indexEmptySlot = NOT_FOUND;

			for(size_t i = 0; i < size(); ++i){
				auto const &x = hh.items[i];

				if (!x){
					indexEmptySlot = i;
					continue;
				}

				if (x.cmpItem(item)){
					if (auto const x_score = x.getScore(); comp(score, x_score))
						return insertItem_(hh.items[i], score);
					else
						return false;
				}

				if (auto const x_score = x.getScore(); comp(minScore, x_score)){
					indexMinScore = i;
					minScore = x_score;
					continue;
				}
			}

			// if we are here, item is not found in the array

			// insert into empty slot
			if (indexEmptySlot != NOT_FOUND)
				return insertItem_(hh.items[indexEmptySlot], score, item);

			// overwrite smallest slot, if compare is OK.
			if (indexMinScore != NOT_FOUND && comp(score, minScore))
				return insertItem_(hh.items[indexMinScore], score, item);

			return false;
		}

	private:
		static bool insertItem_(Item &item, int64_t score){
			item.set(score);

			return true;
		}

		static bool insertItem_(Item &item, int64_t score, std::string_view itemName){
			item.set(score, itemName);

			return true;
		}

	private:
		constexpr static size_t NOT_FOUND = std::numeric_limits<size_t>::max();

		size_t	size_;
	};



	using RawHeavyHitter16   = RawHeavyHitter<  16 - 1>; //   2 x 8
	using RawHeavyHitter32   = RawHeavyHitter<  32 - 1>; //   4 x 8
	using RawHeavyHitter40   = RawHeavyHitter<  40 - 1>; //   5 x 8, IP6 is 8 sets x 4 chars = 32 chars, separated by a colon = 39 chars.
	using RawHeavyHitter64   = RawHeavyHitter<  64 - 1>; //   8 x 8
	using RawHeavyHitter128  = RawHeavyHitter< 128 - 1>; //  16 x 8
	using RawHeavyHitter256  = RawHeavyHitter< 256 - 1>; //  32 x 8
	using RawHeavyHitter512  = RawHeavyHitter< 512 - 1>; //  64 x 8
	using RawHeavyHitter1024 = RawHeavyHitter<1024 - 1>; // 128 x 8

} // namespace heavy_hitter

#endif
