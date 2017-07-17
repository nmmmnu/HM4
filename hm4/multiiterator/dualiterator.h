#ifndef _DUAL_MULTI_TABLE_ITERATOR_H
#define _DUAL_MULTI_TABLE_ITERATOR_H

#include "basemultiiterator_.h"

namespace hm4{
namespace multiiterator{

template <class TABLE1, class TABLE2>
class DualIterator{
public:
	using value_type = const Pair;

private:
	using IteratorPair1	= multiiterator_impl::IteratorPair_<TABLE1>;
	using IteratorPair2	= multiiterator_impl::IteratorPair_<TABLE2>;

public:
	DualIterator(const TABLE1 &table1, const TABLE2 &table2, bool endIt);
	DualIterator(const TABLE1 &table1, const TABLE2 &table2, const StringRef &key);

	DualIterator &operator++();

	const Pair &operator*() const;

	bool operator==(const DualIterator &other) const;

public:
	bool operator!=(const DualIterator &other) const{
		return ! operator==(other);
	}

	const Pair *operator ->() const{
		return & operator*();
	}

private:
	IteratorPair1	it1_;
	IteratorPair2	it2_;
};

} // namespace multiiterator
} // namespace


#include "dualiterator_impl.h"


#endif

