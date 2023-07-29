#ifndef PAIR_VECTOR_H_
#define PAIR_VECTOR_H_

#include "ilist.h"
#include "listcounter.h"
#include "pointer_iterator.h"

namespace hm4{

	template<class T_Allocator>
	class PairVector{
		constexpr static size_t capacity__ = 1024;

	public:
		using Allocator		= T_Allocator;
		using size_type		= config::size_type;
		using difference_type	= config::difference_type;

		using iterator		= pointer_iterator<const Pair * const *>;

	private:
		size_type	size_	= 0;

		Pair *		data_[capacity__];

	public:
		// (MANUAL) D-TOR

		bool destruct(Allocator &allocator) noexcept;

		void clear(Allocator &allocator) noexcept{
			destruct(allocator);
			size_ = 0;
		}

	public:
		constexpr size_type size() const noexcept{
			return size_;
		}

		constexpr size_type capacity() const noexcept{
			return capacity__;
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

		template<class PFactory>
		iterator insertF(PFactory &factory, Allocator &allocator, std::nullptr_t){
			// used for testing
			ListCounter lc;
			return insertF(factory, allocator, lc);
		}

		bool erase_(std::string_view const &key, Allocator &allocator, ListCounter *lc = nullptr);

		void split(PairVector &other);
		void merge(PairVector &other);

	public:
		template<bool B>
		iterator find(std::string_view const key, std::bool_constant<B> exact) const noexcept;

		iterator begin() const noexcept{
			return begin_();
		}

		iterator end() const noexcept{
			return end_();
		}

	private:
		Pair **begin_() noexcept{
			return data_;
		}

		Pair **end_() noexcept{
			return data_ + size_;
		}

		Pair * const *begin_() const noexcept{
			return data_;
		}

		Pair * const *end_() const noexcept{
			return data_ + size_;
		}

		void assign_(Pair **first, Pair **last);
	};

} // namespace hm4

#endif



