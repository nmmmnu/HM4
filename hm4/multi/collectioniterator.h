#ifndef _COLLECTION_MULTI_TABLE_ITERATOR_H
#define _COLLECTION_MULTI_TABLE_ITERATOR_H

#include "basemultiiterator_.h"

#include <vector>

namespace hm4{
namespace multi{


template <class TABLE>
class CollectionIterator : public basemultiiterator_impl_::MultiIteratorTags_{
private:
	using IteratorPair	= basemultiiterator_impl_::IteratorPair_<TABLE>;
	using vector_type	= std::vector<IteratorPair>;

	using size_type		= typename vector_type::size_type;

private:
	template <class CONTAINER, typename T>
	CollectionIterator(const CONTAINER &list, const T &iteratorParam, std::nullptr_t);

public:
	template<class CONTAINER, bool B>
	CollectionIterator(const CONTAINER &list, const base_iterator<B> &tag) :
					CollectionIterator(list, tag, nullptr){}

	template<class CONTAINER>
	CollectionIterator(const CONTAINER &list, const StringRef &key) :
					CollectionIterator(list, key, nullptr){}

	CollectionIterator &operator++();

	const Pair &operator*() const{
		return *dereference_();
	}

	const Pair *operator ->() const{
		return dereference_();
	}

	bool operator==(const CollectionIterator &other) const{
		return it_ == other.it_;
	}

public:
	bool operator!=(const CollectionIterator &other) const{
		return ! operator==(other);
	}

private:
	const Pair *dereference_() const;

private:
	vector_type		it_;

	bool			ended_		= false;

private:
	/* !!! */
	mutable
	const Pair		*tmp_pair	= nullptr;

	mutable
	std::vector<size_type>	tmp_index_pp;

	mutable
	std::vector<size_type>	tmp_index_de;
};


} // namespace multi
} // namespace


#include "collectioniterator.h.cc"


#endif

