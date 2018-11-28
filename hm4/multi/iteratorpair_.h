#ifndef _MULTI_TABLE_ITERATOR_MATRIX_HELPER_H
#define _MULTI_TABLE_ITERATOR_MATRIX_HELPER_H

#include "pair.h"

#include <type_traits>

namespace hm4{
namespace multi{
namespace multiiterator_impl_{

	template <class TABLE>
	class IteratorPair{
	private:
		using iterator = typename TABLE::iterator;

	private:
		template<class T>
		using it_traits = std::iterator_traits<typename T::iterator>;

		template<class T>
		constexpr static bool is_forward_iterator_v =
			std::is_base_of<
				std::forward_iterator_tag,
				typename it_traits<T>::iterator_category
			>::value
		;

		static_assert(is_forward_iterator_v<TABLE>, "TABLE::iterator is not forward_iterator");

	public:
		IteratorPair(iterator &&cur, iterator &&end) :
				cur(std::move(cur)),
				end(std::move(end)){}


		IteratorPair(TABLE const &table, std::true_type) :
				IteratorPair(
					table.begin(),
					table.end()
				){}

		IteratorPair(TABLE const &table, std::false_type) :
				IteratorPair(
					table.end(),
					table.end()
				){}

		IteratorPair(TABLE const &table, StringRef const &key) :
				IteratorPair(
					table.lowerBound(key),
					table.end()
				){}

	public:
		const Pair *dptr() const{
			return cur != end ? & (*cur) : nullptr;
		}

		void operator++(){
			++cur;
		}

		bool operator==(IteratorPair const &other) const{
			return cur == other.cur && end == other.end;
		}

	public:
		iterator cur;
		iterator end;
	};

} // namespace multiiterator_impl_

} // namespace multi
} // namespace


#endif

