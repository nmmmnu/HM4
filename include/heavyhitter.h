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

	} // namespace heavy_hitter_impl_

	template<uint8_t MAX_ITEM_SIZE>
	struct RawHeavyHitter{

		template<bool Up>
		using Comparator = heavy_hitter_impl_::Comparator<Up>;

		struct Item{
			uint64_t score;			//  8
			uint8_t  size;			//  1
			char item[MAX_ITEM_SIZE];	// 63, 127...

			// -----

			constexpr bool valid() const{
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
						size
				};
			}

			void setItem(std::string_view item){
				assert(item.size() <= MAX_ITEM_SIZE);

				size = static_cast<uint8_t>(item.size());

				memcpy(this->item, item.data(), item.size());
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

		static_assert( sizeof(Item) == sizeof(int64_t) + MAX_ITEM_SIZE + 1 );
		static_assert( sizeof(Item[10]) % sizeof(int64_t) == 0 );

		// --------------------------

		constexpr RawHeavyHitter(uint8_t size) : size_(size){
			assert(size != NOT_FOUND);
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

		template<bool Up>
		bool add(Item *M, std::string_view item, int64_t score) const{
			Comparator<Up> const comp;

			int64_t minScore       = comp.start;

			size_t   indexMinScore  = NOT_FOUND;
			size_t   indexEmptySlot = NOT_FOUND;

			for(size_t i = 0; i < size(); ++i){
				auto const &x = M[i];

				if (!x.valid()){
					indexEmptySlot = i;
					continue;
				}

				if (item == x.getItem()){
					if (auto const x_score = x.getScore(); comp(score, x_score))
						return insertItem_(M[i], score);
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
				return insertItem_(M[indexEmptySlot], score, item);

			// overwrite smallest slot, if compare is OK.
			if (indexMinScore != NOT_FOUND && comp(score, minScore))
				return insertItem_(M[indexMinScore], score, item);

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
		constexpr static uint8_t NOT_FOUND = std::numeric_limits<uint8_t>::max();

		uint8_t	size_;
	};



	using RawHeavyHitter32  = RawHeavyHitter< 32 - 1>; //  4 x 8
	using RawHeavyHitter40  = RawHeavyHitter< 40 - 1>; //  5 x 8, IP6
	using RawHeavyHitter64  = RawHeavyHitter< 64 - 1>; //  8 x 8
	using RawHeavyHitter128 = RawHeavyHitter<128 - 1>; // 16 x 8

} // namespace heavy_hitter

#endif


#if 0

		void print(const Item *M) const{
			logger<Logger::DEBUG>() << "HH Dump";

			for(size_t i = 0; i < size(); ++i){
				auto const &x = M[i];

				if (!x.valid()){
					logger<Logger::DEBUG>()
						<< "id" << i
						<< "(empty)"
					;
				}else{
					logger<Logger::DEBUG>()
						<< "id" << i
						<< "score" << x.getScore()
						<< "item" << x.getItem()
					;
				}
			}
		}

#endif


