#ifndef _DUAL_MULTI_TABLE_ITERATOR_H
#define _DUAL_MULTI_TABLE_ITERATOR_H

#include "basemultiiterator_.h"

namespace hm4{
namespace multi{

template <class TABLE1, class TABLE2>
class DualIterator : public impl_::MultiIteratorTags_{
public:
	using value_type = const Pair;

private:
	using IteratorPair1	= impl_::IteratorPair_<TABLE1>;
	using IteratorPair2	= impl_::IteratorPair_<TABLE2>;

public:
	template<bool B>
	DualIterator(const TABLE1 &table1, const TABLE2 &table2, const base_iterator<B> &tag) :
					it1_(table1, tag),
					it2_(table2, tag){}

	DualIterator(const TABLE1 &table1, const TABLE2 &table2, const StringRef &key) :
					it1_(table1, key),
					it2_(table2, key){}

	DualIterator &operator++();

	const Pair &operator*() const;

	bool operator==(const DualIterator &other) const{
		return it1_ == other.it1_ && it2_ == other.it2_;
	}

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

} // namespace multi
} // namespace


#include "dualiterator.h.cc"


#endif

