#ifndef S_LIST_H_
#define S_LIST_H_

#include <cstdint>
#include <cstring>
#include <cassert>
#include <limits>

#include <string_view>

#include "myendian.h"

namespace s_list{
	namespace s_list_impl_{

		struct Item{
			using size_type	= uint16_t;

			size_type	size;
			char		data[1];

			constexpr static auto max_size(){
				return std::numeric_limits<size_type>::max();
			}

			constexpr static auto bytes(size_t size){
				return sizeof(Item) - 1 + size;
			}

			constexpr static auto bytes(std::string_view sv){
				return bytes(sv.size());
			}

			constexpr auto bytes() const{
				return bytes(betoh(size));
			}

			constexpr auto getItem() const{
				return std::string_view{ data, betoh(size) };
			}

			void setItem(std::string_view sv){
				assert(sv.size() < Item::max_size());

				size  = htobe(
						static_cast<size_type>(sv.size())
				);

				memcpy(data, sv.data(), sv.size());
			}

		}__attribute__((__packed__));


		struct List{
			using size_type = uint32_t;

			uint32_t	size;
			uint32_t	foot;
			char		data[1];

			constexpr static auto max_size(){
				return std::numeric_limits<size_type>::max();
			}

			constexpr static auto bytes(size_t items){
				return sizeof(List) - 1 + Item::bytes(0) * items;
			}

		}__attribute__((__packed__));

		template<typename It, typename Projection>
		constexpr auto constexprAccumulate(It first, It last, Projection proj){
			size_t bytes = 0;

			for(auto it = first; it != last; ++it){
				auto const size = proj(*it);

				assert(size < Item::max_size());

				bytes += Item::bytes( size );
			}

			return bytes;
		}

		struct Projection{
			template<typename T>
			constexpr auto operator()(T x){
				return x;
			}

			constexpr auto operator()(std::string_view sv){
				return sv.size();
			}
		};

	} // namespace s_list_impl_

	struct RawSList{
		using Item	= s_list_impl_::Item;
		using List	= s_list_impl_::List;

		static_assert(std::is_trivial<List>::value, "List must be POD type");

		using size_type	= typename List::size_type;

		RawSList(size_type capacity) : capacity_(capacity - List::bytes(0)){
			assert(capacity > List::bytes(1));
		}

		RawSList(size_t capacity) : RawSList(static_cast<size_type>(capacity)){
			assert(capacity < List::max_size());
		}

		size_type size(List const &list) const{
			return betoh(list.size);
		}

		constexpr size_type capacity() const{
			return capacity_;
		}

		constexpr void clear(List &list) const{
			list.size = 0;
			list.foot = 0;
		}

		constexpr static bool isItemValid(std::string_view sv){
			return sv.size() >= 1 && sv.size() <= Item::max_size();
		}

	public:
		// calculate the size of new object

		constexpr static size_t bytes(size_t size){
			assert(size < Item::max_size());

			return bytes_(Item::bytes(size));
		}

		constexpr static size_t bytes(std::string_view sv){
			return bytes(sv.size());
		}

		template<typename It, typename Projection>
		constexpr static size_t bytes(It first, It last, Projection proj){
			assert(std::distance(first, last) < List::max_size());

			return bytes_(
				s_list_impl_::constexprAccumulate(first, last, proj)
			);
		}

		template<typename It>
		constexpr static size_t bytes(It first, It last){
			return bytes(first, last, s_list_impl_::Projection{});
		}

	public:
		// calculate the size of existing object

		constexpr size_t bytes(List const &list, size_t size) const{
			assert(size < Item::max_size());

			return bytes_(list, Item::bytes(size));
		}

		constexpr size_t bytes(List const &list, std::string_view sv) const{
			return bytes(list, sv.size());
		}

		template<typename It, typename Projection>
		constexpr size_t bytes(List const &list, It first, It last, Projection proj) const{
			assert(std::distance(first, last) < List::max_size());

			return bytes_(
				list,
				s_list_impl_::constexprAccumulate(first, last, proj)
			);
		}

		template<typename It>
		constexpr size_t bytes(List const &list, It first, It last) const{
			return bytes(list, first, last, s_list_impl_::Projection{});
		}

	public:
		bool push(List &list, std::string_view sv) const{
			assert(sv.size() < Item::max_size());

			if (!safe_(list)){
				// invalid state
				clear(list);
			}

			auto const listSize = betoh(list.size);

			if (listSize == List::max_size())
				return false;

			auto const foot  = betoh(list.foot);

			char *mem = &list.data[foot];

			// positioned at the end

			size_type const need = static_cast<size_type>(Item::bytes(sv));

			if (!safeNext_(list, mem, need))
				return false;

			auto *item = reinterpret_cast<Item *>(mem);

			item->setItem(sv);

			list.size = htobe(listSize + 1);
			list.foot = htobe(foot + need);

			return true;
		}

		bool push(List &list, std::string_view sv, size_t maxSize) const{
			if (sv.size() > maxSize)
				return false;

			return push(list, sv);
		}

	public:
		template<typename F>
		bool for_each(List const &list, F f) const{
			if (!safe_(list)){
				// invalid state
				return false;
			}

			auto const foot = betoh(list.foot);

			for(const char *mem  = list.data; mem < list.data + foot;){
				const char *next = safeNext_(list, mem);

				if (!next){
					// invalid state
					return false;
				}

				if (! f(*reinterpret_cast<const Item *>(mem)) )
					return false;

				mem = next;
			}

			return true;
		}

	private:
		constexpr static size_t bytes_(size_t bytes){
			return List::bytes(0) + bytes;
		}

		constexpr size_t bytes_(List const &list, size_t bytes) const{
			if (!safe_(list)){
				// invalid state, push will do clear()
				return bytes_(
					bytes <= capacity_ ? capacity_ : bytes
				);
			}

			auto const foot = betoh(list.foot);

			bytes += foot;

			return bytes_(
				bytes <= capacity_ ? capacity_ : bytes * CAPACITY_MULTIPLIER
			);
		}

	private:
		constexpr bool safe_(List const &list) const{
			//auto const size = betoh(list.size);

			auto const foot = betoh(list.foot);

			return foot <= capacity_;
		}

		template<typename Char>
		Char *safeNext_(List const &list, Char *mem, size_t size) const{
			const char *max = list.data + capacity_;

			return mem + size <= max ? mem + size : nullptr;
		}

		template<typename Char>
		Char *safeNext_(List const &list, Char *mem) const{
			const char *max = list.data + capacity_;

			if (mem + Item::bytes(0) > max)
				return nullptr;

			const auto *item = reinterpret_cast<const Item *>(mem);

			return safeNext_(list, mem, item->bytes());
		}

	private:
		size_type capacity_;

		constexpr static size_type CAPACITY_MULTIPLIER = 2;
	};

} // namespace s_list

#endif





