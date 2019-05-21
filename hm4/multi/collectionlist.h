#ifndef COLLECTION_LIST_H_
#define COLLECTION_LIST_H_

#include "ilist.h"

#include "collectioniterator.h"

#include <numeric>	// std::accumulate

namespace hm4{
namespace multi{
namespace icollectionlist_impl_{

template <class StoreIterator, class P>
class iCollectionList{
public:
	using List		= typename std::iterator_traits<StoreIterator>::value_type;
	using StoreIteratorDT	= typename std::iterator_traits<StoreIterator>::difference_type;

	using size_type		= typename List::size_type;
	using difference_type	= typename List::difference_type;

	using iterator		= CollectionIterator<typename List::iterator>;

	using estimated_size	= std::true_type;

private:
	template<class T>
	constexpr static bool is_forward_iterator_v =
		std::is_base_of<
			std::forward_iterator_tag,
			typename std::iterator_traits<T>::iterator_category
		>::value
	;

	static_assert(is_forward_iterator_v<StoreIterator>, "Iterator is not forward_iterator");

private:
	const P *self() const{
		return static_cast<const P*>(this);
	}

	StoreIterator first_() const{
		return self()->first_();
	}

	StoreIterator last_() const{
		return self()->last_();
	}

public:
	iterator begin() const{
		return { first_(), last_(), std::true_type{} };
	}

	iterator end() const{
		return { first_(), last_(), std::false_type{} };
	}

	template<bool B>
	iterator find(StringRef const &key, std::bool_constant<B> const exact) const{
		return { first_(), last_(), key, exact };
	}

public:
	size_type size() const{
		auto sum = [](size_t const result, List const &list){
			return result + list.size();
		};

		return std::accumulate(first_(), last_(), size_t{ 0 }, sum);
	}

	size_t bytes() const{
		auto sum = [](size_type const result, List const &list){
			return result + list.bytes();
		};

		return std::accumulate(first_(), last_(), size_type{ 0 }, sum);
	}
};


} // namespacecollectionlist_impl_



template <class StoreIterator>
class CollectionListFromIterator : public icollectionlist_impl_::iCollectionList<StoreIterator, CollectionListFromIterator<StoreIterator> >{
private:
	StoreIterator		firstIt_;
	StoreIterator		lastIt_;

public:
	CollectionListFromIterator(StoreIterator first, StoreIterator last) :
					firstIt_	(std::move(first	)),
					lastIt_		(std::move(last		)){}

public:
	auto first_() const{
		return firstIt_;
	}

	auto last_() const{
		return lastIt_;
	}
};



template <class StoreContainer>
class CollectionListFromContainer : public icollectionlist_impl_::iCollectionList<typename StoreContainer::const_iterator, CollectionListFromContainer<StoreContainer> >{
private:
	const StoreContainer	*list_;

public:
	CollectionListFromContainer(const StoreContainer &list) :
					list_	(&list){}

public:
	auto first_() const{
		return std::begin(*list_);
	}

	auto last_() const{
		return std::end(*list_);
	}
};

} // namespace multi
} // namespace

#endif

