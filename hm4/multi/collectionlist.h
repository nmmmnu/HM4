#ifndef COLLECTION_LIST_H_
#define COLLECTION_LIST_H_

#include "ilist.h"

#include "collectioniterator.h"

#include <algorithm>    // std::count


namespace hm4{
namespace multi{


template <class CONTAINER>
class CollectionList : public IList<CollectionList<CONTAINER>, false>{
public:
	using List	= typename CONTAINER::value_type;

	using Iterator	= CollectionIterator<List>;

	using size_type	= typename CollectionList::size_type;

public:
	CollectionList(const CONTAINER &container) : container_(container){}

public:
	Iterator begin() const{
		return Iterator(container_, typename Iterator::begin_iterator{} );
	}

	Iterator end() const{
		return Iterator(container_, typename Iterator::end_iterator{}	 );
	}

	Iterator lowerBound(const StringRef &key) const{
		return Iterator(container_, key);
	}

public:
	const Pair *operator[](const StringRef &key) const;

	size_type size(bool const estimated = false) const{
		return estimated ? sizeEstimated_(true) : this->sizeViaIterator_();
	}

	size_t bytes() const;

private:
	size_type sizeEstimated_(bool estimated) const;

private:
	const CONTAINER	&container_;
};


} // namespace multi
} //namespace

// ===================================

#include "collectionlist.h.cc"

#endif

