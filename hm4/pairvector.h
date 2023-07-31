#ifndef PAIR_VECTOR_H_
#define PAIR_VECTOR_H_

#include "ilist.h"
#include "listcounter.h"
#include "pointer_iterator.h"

namespace hm4{

	template<class Allocator, size_t Capacity = 1024>
	class PairVector{
		constexpr static size_t capacity__ = Capacity;

	public:
		using size_type		= config::size_type;
		using difference_type	= config::difference_type;

		using iterator		= pointer_iterator<const Pair * const *>;

		using const_ptr_iterator		= Pair * const *;
		using ptr_iterator		= Pair **;

	private:
		size_type	size_	= 0;

		Pair *		data_[capacity__];

	public:
		// (Manual) c-tor

		void construct() noexcept{
			return zeroing_();
		}

		// (Manual) d-tor

		bool destruct(Allocator &allocator) noexcept;

		void clear(Allocator &allocator) noexcept{
			destruct(allocator);
			zeroing_();
		}

	public:
		constexpr size_type size() const noexcept{
			return size_;
		}

		constexpr size_type capacity() const noexcept{
			return capacity__;
		}

		constexpr bool full() const noexcept{
			return size() == capacity();
		}

		constexpr Pair const &operator[](size_type const index) const noexcept{
			return *data_[index];
		}

		constexpr Pair const &front() const noexcept{
			return *data_[0];
		}

		constexpr Pair const &back() const noexcept{
			return *data_[size_ - 1];
		}

		template<class PFactory>
		iterator insertF(PFactory &factory, Allocator &allocator, ListCounter &lc);

		// used for testing
		template<class PFactory>
		iterator insertF(PFactory &factory, Allocator &allocator, std::nullptr_t){
			ListCounter lc;
			return insertF(factory, allocator, lc);
		}

		bool erase_(std::string_view const &key, Allocator &allocator, ListCounter &lc);

		// used for testing
		bool erase_(std::string_view const &key, Allocator &allocator){
			ListCounter lc;
			return erase_(key, allocator, lc);
		}

		void split(PairVector &other);
		void merge(PairVector &other);

	public:
		struct ConstLocateResultPtr{
			bool			found;
			const_ptr_iterator	it;
		};

		const_ptr_iterator ptr_begin() const noexcept{
			return data_;
		}

		const_ptr_iterator ptr_end() const noexcept{
			return data_ + size_;
		}

		ConstLocateResultPtr locateC_(std::string_view const key) const noexcept;

	public:
		struct LocateResultPtr{
			bool			found;
			ptr_iterator		it;
		};

		ptr_iterator ptr_begin() noexcept{
			return data_;
		}

		ptr_iterator ptr_end() noexcept{
			return data_ + size_;
		}

		LocateResultPtr locateM_(std::string_view const key) noexcept;

	public:
		template<bool B>
		iterator find(std::string_view const key, std::bool_constant<B> exact) const noexcept;

		constexpr iterator begin() const noexcept{
			return iterator{ ptr_begin() };
		}

		constexpr iterator end() const noexcept{
			return iterator{ ptr_end() };
		}

	private:
		void assign_(Pair **first, Pair **last);

	private:
		void zeroing_(){
			size_ = 0;
		}
	};

} // namespace hm4

#endif



