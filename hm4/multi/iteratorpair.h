#ifndef _MULTI_TABLE_ITERATOR_MATRIX_HELPER_H
#define _MULTI_TABLE_ITERATOR_MATRIX_HELPER_H

#include "pair.h"

#include <type_traits>
#include "my_type_traits.h"

namespace hm4{
namespace multi{
namespace multiiterator_impl_{

	template <class Iterator>
	class IteratorPair{
	private:
		using iterator = Iterator;

	private:
		template<class T>
		constexpr static bool is_forward_iterator_v =
			std::is_base_of<
				std::forward_iterator_tag,
				typename std::iterator_traits<T>::iterator_category
			>::value
		;

		static_assert(is_forward_iterator_v<Iterator>, "iterator is not forward_iterator");

	public:
		IteratorPair(iterator cur, iterator end) :
				cur(std::move(cur)),
				end(std::move(end)){}

		template<class List, bool B>
		IteratorPair(List const &list, std::bool_constant<B> const tag) :
				IteratorPair(getIterator(list, tag), std::end(list)){}

		template<class List, bool B>
		IteratorPair(List const &list, std::string_view const key, std::bool_constant<B> const exact) :
				IteratorPair(getIterator(list, key, exact), std::end(list)){}

	public:
		operator bool() const{
			return cur != end;
		}

		const Pair &operator*() const{
			return *cur;
		}

		const Pair *operator->() const{
			return & operator*();
		}

		const Pair *ptr() const{
			return cur != end ? & *cur : nullptr;
		}

		void operator++(){
			++cur;
		}

		bool operator==(IteratorPair const &other) const{
			return cur == other.cur && end == other.end;
		}

	private:
		iterator cur;
		iterator end;
	};



	template <class Iterator1, class Iterator2>
	int compNonEmpty(IteratorPair<Iterator1> const &a, IteratorPair<Iterator2> const &b, bool const fullTimeCompare){
		int const cmp = a->cmp(b->getKey());

		if (fullTimeCompare == false || cmp != 0)
			return cmp;

		// return bigger time or first
		return - a->cmpTime(*b);
	}

	template <class Iterator1, class Iterator2>
	int comp(IteratorPair<Iterator1> const &a, IteratorPair<Iterator2> const &b, bool const fullTimeCompare = true){
		if (a == false && b == false)
			return 0;

		if (a == false)
			return +1;

		if (b == false)
			return -1;

		return compNonEmpty(a, b, fullTimeCompare);
	}
} // namespace multiiterator_impl_

} // namespace multi
} // namespace


#endif

