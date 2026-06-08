#ifndef ring_buffer_h_
#define ring_buffer_h_

#include <cstdint>
#include <cassert>
#include <cstring>
#include <string_view>

#include "myendian.h"

namespace ring_buffer{
	namespace ring_buffer_impl_{
		template<typename SizeType, SizeType MaxItemSize>
		class Item_{
			SizeType	size;			//  1, 2, 4, 8
			char		item[MaxItemSize];	// 63, 127...

		public:
			constexpr operator bool() const{
				return size;
			}

			constexpr auto getItem() const{
				if (auto const s = betoh(size); s <= MaxItemSize)
					return std::string_view{ item, s };
				else
					return std::string_view{};
			}

			void setItem(std::string_view sv){
				assert(sv.size() <= MaxItemSize);

				size  = htobe(
						static_cast<SizeType>(sv.size())
				);

				memcpy(item, sv.data(), sv.size());
			}

			constexpr void clrItem(){
				size = 0;
			}

			constexpr auto getClrItem(){
				auto const s = getItem();

				clrItem();

				return s;
			}
		} __attribute__((__packed__));

		template<typename SizeType, SizeType MaxItemSize>
		struct List{
			using Item	= Item_<SizeType,MaxItemSize>;

			using HSizeType	= uint32_t;

			HSizeType	head;	// write index
			HSizeType	tail;	// read  index
			Item		items[1];
		} __attribute__((__packed__));

	} // namespace ring_buffer_impl_

	template<typename SizeType, SizeType MaxItemSize>
	struct RawRingBuffer{
		using List		= ring_buffer_impl_::List<SizeType,MaxItemSize>;
		using Item		= typename List::Item;

		static_assert(MaxItemSize >= 1);
		static_assert(std::is_trivial<List>::value, "List must be POD type");
		static_assert( sizeof(List) % sizeof(int64_t) == 0 );

	public:
		constexpr RawRingBuffer(size_t size) : size_(size){}

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

		void clear(List *rb) const{
			memset(rb, 0, bytes());
		}

		void load(List *rb, const void *src) const{
			memcpy(rb, src, bytes());
		}

		void store(const List *rb, void *dest) const{
			memcpy(dest, rb, bytes());
		}

		// --------------------------

		constexpr size_t count(List const &rb) const{
			if (!check_(rb))
				return 0;

			auto const head = betoh(rb.head);
			auto const tail = betoh(rb.tail);

			if (head == tail)
				return rb.items[tail] ? size_ : 0;

			if (head > tail)
				return head - tail;
			else
				return (size_ - tail) + head;
		}

		// --------------------------

		void clear(List &rb) const{
			rb.head = 0;
			rb.tail = 0;

			rb.items[0].clrItem();
		}

		void push(List &rb, std::string_view item) const{
			checkSet_(rb);

			auto const head = betoh(rb.head);
			auto const tail = betoh(rb.tail);

			bool const full = head == tail && rb.items[tail];

			rb.items[head].setItem(item);

			auto const nextHead = head + 1 == size_ ? 0 : head + 1;

			if (full){
				auto const nextTail = tail + 1 == size_ ? 0 : tail + 1;

				rb.tail = htobe(nextTail);
			}

			rb.head = htobe(nextHead);
		}

		auto pop(List &rb) const{
			checkSet_(rb);

			auto const head = betoh(rb.head);
			auto const tail = betoh(rb.tail);

			if (head == tail && !rb.items[tail])
				return std::string_view{};

			auto const item = rb.items[tail].getClrItem();

			auto nextTail = tail + 1;

			if (nextTail == size_)
				nextTail = 0;

			rb.tail = htobe(nextTail);

			return item;
		}

		template<typename F>
		void for_each(List const &rb, F f) const{
			if (!check_(rb))
				return;

			auto const head = betoh(rb.head);
			auto const tail = betoh(rb.tail);

			if (head == tail && !rb.items[tail])
				return;

			auto const stepsMax = count(rb);

			auto i = tail;

			for(size_t steps = 0; steps < stepsMax; ++steps){
				f(rb.items[i].getItem());

				i = (i + 1 == size_) ? 0 : i + 1;
			}
		}

	private:
		bool check_(List const &rb) const{
			auto const head = betoh(rb.head);
			auto const tail = betoh(rb.tail);

			return head < size_ && tail < size_;
		}

		void checkSet_(List &rb) const{
			if (check_(rb))
				return;

			clear(rb);
		}

	private:
		size_t	size_;
	};

	template<typename T, auto N>
	using RawRingBuffer__	= RawRingBuffer<T, N - sizeof(T)>;

	using RawRingBuffer16	= RawRingBuffer__< uint8_t,   16>; //   2 x 8
	using RawRingBuffer32	= RawRingBuffer__< uint8_t,   32>; //   4 x 8
	using RawRingBuffer40	= RawRingBuffer__< uint8_t,   40>; //   5 x 8, IP6 is 8 sets x 4 chars = 32 chars, separated by a colon = 39 chars.
	using RawRingBuffer64	= RawRingBuffer__< uint8_t,   64>; //   8 x 8
	using RawRingBuffer128	= RawRingBuffer__< uint8_t,  128>; //  16 x 8
	using RawRingBuffer256	= RawRingBuffer__< uint8_t,  256>; //  32 x 8
	using RawRingBuffer512	= RawRingBuffer__<uint16_t,  512>; //  64 x 8
	using RawRingBuffer1024	= RawRingBuffer__<uint16_t, 1024>; // 128 x 8

} // namespace ring_buffer

#endif

