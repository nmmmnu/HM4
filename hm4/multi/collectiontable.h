#ifndef _LSM_TABLE_H
#define _LSM_TABLE_H

#include "ilist.h"

#include "collectioniterator.h"

#include <algorithm>    // std::count


namespace hm4{
namespace multi{


template <class CONTAINER>
class CollectionTable : public List<CollectionTable<CONTAINER> >{
public:
	using Table	= typename CONTAINER::value_type;

	using Iterator	= CollectionIterator<Table>;

	using size_type	= typename CollectionTable::size_type;

public:
	CollectionTable(const CONTAINER &container) : container_(container){}

public:
	Iterator begin() const{
		return Iterator(container_, typename Iterator::begin_iterator{}	);
	}

	Iterator end() const{
		return Iterator(container_, typename Iterator::end_iterator{}	);
	}

	Iterator lowerBound(const StringRef &key) const{
		return Iterator(container_, key);
	}

public:
	ObserverPair operator[](const StringRef &key) const;

	size_type size(bool const estimated = false) const{
		return estimated ? sizeEstimated_() : sizeReal_();
	}

	size_t bytes() const;

private:
	size_type sizeEstimated_() const;
	size_type sizeReal_() const;

private:
	const CONTAINER	&container_;
};


} // namespace multi
} //namespace

// ===================================

#include "collectiontable_impl.h"

#endif

