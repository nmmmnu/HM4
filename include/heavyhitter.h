#ifndef HEAVY_HITTER_H_
#define HEAVY_HITTER_H_

#include <cstdint>
#include <cassert>
#include <cstring>
#include <string_view>
#include <limits>

#include "myendian.h"
//#include "logger.h"

namespace heavy_hitter{

	template<uint8_t MAX_ITEM_SIZE>
	struct RawHeavyHitter{
		struct Item{
			uint64_t score;			//  8
			uint8_t  size;			//  1
			char key[MAX_ITEM_SIZE];	// 63, 127...

			constexpr std::string_view getItem() const{
				return std::string_view{ key, size };
			}

			void setItem(std::string_view s){
				assert(s.size() <= MAX_ITEM_SIZE);

				size = static_cast<uint8_t>(s.size());

				memcpy(key, s.data(), s.size());
			}

		} __attribute__((__packed__));

		static_assert( sizeof(Item) == sizeof(uint64_t) + MAX_ITEM_SIZE + 1 );
		static_assert( sizeof(Item[10]) % sizeof(uint64_t) == 0 );

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

		bool add(Item *M, std::string_view item, uint64_t score) const{
			uint64_t minScore       = std::numeric_limits<uint64_t>::max();

			size_t   indexMinScore  = NOT_FOUND;
			size_t   indexEmptySlot = NOT_FOUND;

			for(size_t i = 0; i < size(); ++i){
				auto const &x = M[i];

				if (!x.score){
					indexEmptySlot = i;
					continue;
				}

				if (item == x.getItem())
					return hitItem_(M[i], score, M);

				auto const x_score = betoh(x.score);

				if (minScore > x_score){
					indexMinScore = i;
					minScore = x_score;
					continue;
				}
			}

			// if we are here, item is not found in the array

			// insert into empty slot
			if (indexEmptySlot != NOT_FOUND)
				return replaceItem_(M[indexEmptySlot], score, item, M);

			// overwrite smallest slot
			if (indexMinScore != NOT_FOUND && score > minScore)
				return replaceItem_(M[indexMinScore], score, item, M);

			return false;
		}

		void print(const Item *M) const{
			logger<Logger::DEBUG>() << "HH Dump";

			for(size_t i = 0; i < size(); ++i){
				auto const &x = M[i];

				if (!x.score){
					logger<Logger::DEBUG>()
						<< "id" << i
						<< "(empty)"
					;
				}else{
					logger<Logger::DEBUG>()
						<< "id" << i
						<< "score" << betoh(x.score)
						<< "item" << x.getItem()
					;
				}
			}
		}

	private:
		bool hitItem_(Item &item, uint64_t score, const Item *) const{
			if (score <= betoh(item.score))
				return false;

			item.score = htobe(score);

		//	print(all);

			return true;
		}

		bool replaceItem_(Item &item, uint64_t score, std::string_view itemName, const Item *) const{
			if (score <= betoh(item.score))
				return false;

			item.score = htobe(score);
			item.setItem(itemName);

		//	print(all);

			return true;
		}

	private:
		constexpr static uint8_t NOT_FOUND = std::numeric_limits<uint8_t>::max();

		uint8_t	size_;
	};



	using RawHeavyHitter64  = RawHeavyHitter< 64 - 1>;
	using RawHeavyHitter128 = RawHeavyHitter<128 - 1>;

} // namespace heavy_hitter

#endif

