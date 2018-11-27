#ifndef _MULTI_TABLE_ITERATOR_MATRIX_HELPER_H
#define _MULTI_TABLE_ITERATOR_MATRIX_HELPER_H

#include "pair.h"

#include <type_traits>

namespace hm4{
namespace multi{
namespace multiiterator_impl_{

	template <class TABLE>
	class IteratorPair {
	private:
		using Iterator		= typename TABLE::Iterator;

	public:
		IteratorPair(Iterator &&cur, Iterator &&end) :
				cur(std::move(cur)),
				end(std::move(end)){}


		IteratorPair(const TABLE &table, std::true_type) :
				IteratorPair(
					table.begin(),
					table.end()
				){}

		IteratorPair(const TABLE &table, std::false_type) :
				IteratorPair(
					table.end(),
					table.end()
				){}

		IteratorPair(const TABLE &table, StringRef const &key) :
				IteratorPair(
					table.lowerBound(key),
					table.end()
				){}

	public:
		const Pair *dptr() const{
			return cur != end ? & (*cur) : nullptr;
		}

		void operator ++(){
			++cur;
		}

		bool operator==(IteratorPair const &other) const{
			return cur == other.cur && end == other.end;
		}

	public:
		Iterator cur;
		Iterator end;
	};

} // namespace multiiterator_impl_

} // namespace multi
} // namespace


#endif

