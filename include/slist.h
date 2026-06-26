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



	template<bool Mutable>
	struct RawSListBase{
		using Item	= s_list_impl_::Item;
		using List	= s_list_impl_::List;

		static_assert(std::is_trivial_v<List>, "List must be POD type");

		using TList = std::conditional_t<Mutable, List, const List>;

		using size_type	= typename List::size_type;

		constexpr RawSListBase(TList *list, size_t capacity) : list_(list), capacityFull_(capacity){}

		[[nodiscard]]
		constexpr size_type size() const{
			if (!stable_(std::false_type{}))
				return 0;

			return betoh(list_->size);
		}

		[[nodiscard]]
		constexpr size_t capacity() const{
			if (capacityFull_ < List::bytes(0))
				return 0;

			return capacityFull_ - List::bytes(0);
		}

		template <bool M = Mutable, typename = std::enable_if_t<M> >
		[[nodiscard]]
		constexpr bool clear(){
			if (!stable_(std::false_type{}))
				return false;

			list_->size = 0;
			list_->foot = 0;

			return true;
		}

		[[nodiscard]]
		constexpr static bool isItemValid(std::string_view sv){
			return sv.size() >= 1 && sv.size() <= Item::max_size();
		}

	public:
		// calculate the size of new object

		[[nodiscard]]
		constexpr static size_t bytes(size_t size){
			assert(size < Item::max_size());

			return bytes_(Item::bytes(size));
		}

		[[nodiscard]]
		constexpr static size_t bytes(std::string_view sv){
			return bytes(sv.size());
		}

		template<typename It, typename Projection>
		[[nodiscard]]
		constexpr static size_t bytes(It first, It last, Projection proj){
			assert(std::distance(first, last) < List::max_size());

			return bytes_(
				s_list_impl_::constexprAccumulate(first, last, proj)
			);
		}

		template<typename It>
		[[nodiscard]]
		constexpr static size_t bytes(It first, It last){
			return bytes(first, last, s_list_impl_::Projection{});
		}

	public:
		// calculate the size of existing object

		[[nodiscard]]
		constexpr size_t bytes(size_t size, bool capacityMultiply) const{
			assert(size < Item::max_size());

			return bytes_(Item::bytes(size), capacityMultiply);
		}

		[[nodiscard]]
		constexpr size_t bytes(std::string_view sv, bool capacityMultiply) const{
			return bytes(sv.size(), capacityMultiply);
		}

		template<typename It, typename Projection>
		[[nodiscard]]
		constexpr size_t bytes(It first, It last, bool capacityMultiply, Projection proj) const{
			assert(std::distance(first, last) < List::max_size());

			return bytes_(
				s_list_impl_::constexprAccumulate(first, last, proj),
				capacityMultiply
			);
		}

		template<typename It>
		[[nodiscard]]
		constexpr size_t bytes(It first, It last, bool capacityMultiply) const{
			return bytes(first, last, capacityMultiply, s_list_impl_::Projection{});
		}

	public:
		template <bool M = Mutable, typename = std::enable_if_t<M> >
		bool push(std::string_view sv){
			assert(sv.size() < Item::max_size());

			if (!stable_(std::true_type{})){
				// invalid state, try to repair
				if (!clear())
					return false;
			}

			auto const listSize = betoh(list_->size);

			if (listSize == List::max_size())
				return false;

			auto const foot  = betoh(list_->foot);

			char *mem = &list_->data[foot];

			// positioned at the end

			size_type const need = static_cast<size_type>(Item::bytes(sv));

			if (!safeNext_(mem, need))
				return false;

			auto *item = reinterpret_cast<Item *>(mem);

			item->setItem(sv);

			list_->size = htobe(listSize + 1);
			list_->foot = htobe(foot + need);

			return true;
		}

		template <bool M = Mutable, typename = std::enable_if_t<M> >
		bool push(std::string_view sv, size_t maxSize){
			if (sv.size() > maxSize)
				return false;

			return push(sv);
		}

	public:
		template<typename F>
		constexpr bool for_each(F f) const{
			if (!stable_(std::true_type{})){
				// invalid state
				return false;
			}

			auto const foot = betoh(list_->foot);

			for(const char *mem  = list_->data; mem < list_->data + foot;){
				const char *next = safeNext_(mem);

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
		[[nodiscard]]
		constexpr static size_t bytes_(size_t bytes){
			return List::bytes(0) + bytes;
		}

		[[nodiscard]]
		constexpr size_t bytes_(size_t bytes, bool capacityMultiply) const{
			if (!stable_(std::true_type{})){
				// invalid state, push will do clear()
				return bytes_(
					bytes <= capacity() ? capacity() : bytes
				);
			}

			auto const foot = betoh(list_->foot);

			bytes += foot;

			size_type const cm = capacityMultiply ? 2 : 1;

			return bytes_(
				bytes <= capacity() ? capacity() : bytes * cm
			);
		}

	private:
		template<bool B>
		[[nodiscard]]
		constexpr bool stable_(std::bool_constant<B>) const{
			if (!list_)
				return false;

			if (!capacity())
				return false;

			if constexpr(B){
				auto const foot = betoh(list_->foot);

				return foot <= capacity();
			}else{
				return true;
			}
		}

		template<typename Char>
		[[nodiscard]]
		Char *safeNext_(Char *mem, size_t size) const{
			const char *max = list_->data + capacity();

			return mem + size <= max ? mem + size : nullptr;
		}

		template<typename Char>
		[[nodiscard]]
		Char *safeNext_(Char *mem) const{
			const char *max = list_->data + capacity();

			if (mem + Item::bytes(0) > max)
				return nullptr;

			const auto *item = reinterpret_cast<const Item *>(mem);

			return safeNext_(mem, item->bytes());
		}

	private:
		TList	*list_;
		size_t	capacityFull_;
	};

	using RawSListConst	= RawSListBase<false>;
	using RawSList		= RawSListBase<true>;

} // namespace s_list

#endif





