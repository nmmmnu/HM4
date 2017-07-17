#ifndef _MULTI_TABLE_ITERATOR_MATRIX_HELPER_H
#define _MULTI_TABLE_ITERATOR_MATRIX_HELPER_H

#include "pair.h"

namespace hm4{
namespace multiiterator{


namespace multiiterator_impl{

	template <class TABLE>
	class IteratorPair_{
	private:
		using Iterator		= typename TABLE::Iterator;

	public:
		IteratorPair_(Iterator &&cur, Iterator &&end) :
						cur(std::move(cur)),
						end(std::move(end)){}

		IteratorPair_(const TABLE &table, bool const endIt = false) :
						IteratorPair_(
							endIt ? table.end() : table.begin(),
							table.end()
						){}

		IteratorPair_(const TABLE &table, const StringRef &key) :
						IteratorPair_(
							table.lowerBound(key),
							table.end()
						){}

	public:
		const Pair &operator *() const{
			return cur != end ? *cur : Pair::zero();
		}

		void operator ++(){
			++cur;
		}

		bool operator==(const IteratorPair_ &other) const{
			return cur == other.cur && end == other.end;
		}

		bool operator!=(const IteratorPair_ &other) const{
			return ! operator==(other);
		}

	public:
		Iterator cur;
		Iterator end;
	};

} // namespace base


} // namespace multitableiterator
} // namespace

#endif

