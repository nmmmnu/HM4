#ifndef _MULTI_TABLE_ITERATOR_MATRIX_HELPER_H
#define _MULTI_TABLE_ITERATOR_MATRIX_HELPER_H

#include "pair.h"

namespace hm4{
namespace multiiterator{


namespace impl_{


	struct MultiIteratorTags_{
		struct begin_iterator{};
		struct end_iterator{};
	};

	// ===================================

	template <class TABLE>
	class IteratorPair_ : public MultiIteratorTags_{
	private:
		using Iterator		= typename TABLE::Iterator;

	public:
		IteratorPair_(Iterator &&cur, Iterator &&end) :
						cur(std::move(cur)),
						end(std::move(end)){}

		IteratorPair_(const TABLE &table, const begin_iterator&) :
						IteratorPair_(
							table.begin(),
							table.end()
						){}

		IteratorPair_(const TABLE &table, const end_iterator&) :
						IteratorPair_(
							table.end(),
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

	public:
		Iterator cur;
		Iterator end;
	};

} // namespace impl_


} // namespace multiiterator
} // namespace


#endif

