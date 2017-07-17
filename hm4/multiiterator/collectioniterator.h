#ifndef _COLLECTION_MULTI_TABLE_ITERATOR_H
#define _COLLECTION_MULTI_TABLE_ITERATOR_H

#include "basemultiiterator_.h"

#include <vector>

namespace hm4{
namespace multiiterator{


template <class TABLE>
class CollectionIterator : public impl_::MultiIteratorTags_{
private:
	using IteratorPair	= impl_::IteratorPair_<TABLE>;
	using vector_type	= std::vector<IteratorPair>;

	using size_type		= typename vector_type::size_type;

private:
	template <class CONTAINER, typename T>
	CollectionIterator(const CONTAINER &list, const T &iteratorParam, std::nullptr_t);

public:
	template<class CONTAINER>
	CollectionIterator(const CONTAINER &list, const begin_iterator &tag) :
					CollectionIterator(list, tag, nullptr){}

	template<class CONTAINER>
	CollectionIterator(const CONTAINER &list, const end_iterator &tag) :
					CollectionIterator(list, tag, nullptr){}

	template<class CONTAINER>
	CollectionIterator(const CONTAINER &list, const StringRef &key) :
					CollectionIterator(list, key, nullptr){}

	CollectionIterator &operator++();

	const Pair &operator*() const{
		return tmp_pair ? *tmp_pair : dereference_();
	}

	bool operator==(const CollectionIterator &other) const{
		return it_ == other.it_;
	}

public:
	bool operator!=(const CollectionIterator &other) const{
		return ! operator==(other);
	}

	const Pair *operator ->() const{
		return & operator*();
	}

private:
	const Pair &dereference_() const;

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


} // namespace multitableiterator
} // namespace


#include "collectioniterator_impl.h"


#endif

