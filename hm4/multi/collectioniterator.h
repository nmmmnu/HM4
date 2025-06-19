#ifndef _COLLECTION_MULTI_TABLE_ITERATOR_H
#define _COLLECTION_MULTI_TABLE_ITERATOR_H

#include "iteratorpair.h"

//#include <vector>
#include <algorithm>	// heap
#include "smallvector.h"

namespace hm4{
namespace multi{



template <class Iterator>
class CollectionIterator{
public:
	CollectionIterator(CollectionIterator const &other) = default;
	CollectionIterator(CollectionIterator &&other) = default;
	CollectionIterator &operator =(CollectionIterator &&other) = default;

public:
	using difference_type	= typename std::iterator_traits<Iterator>::difference_type;
	using value_type	= const Pair;
	using pointer		= value_type *;
	using reference		= value_type &;
	using iterator_category	= std::forward_iterator_tag;

private:
	using ITP		= multiiterator_impl_::IteratorPair<Iterator>;

	// sizeof(ITP) => 64

	constexpr static size_t ITPVectorInlineCapacity = 64;

	//using ITPVector	= std::vector<ITP>;
	using ITPVector		= SmallVector<ITP, ITPVectorInlineCapacity>; // as of 2025-06-19, sizeof 4120

	ITPVector		itp_;

public:
	template<class StoreIterator, typename ...Args>
	CollectionIterator(
			StoreIterator first, StoreIterator last,
			Args&& ...args
	){
		auto const size = static_cast<typename ITPVector::size_type>( std::distance(first, last) );

		itp_.reserve(size);

		// if (size <= itp_.capacity()){
		// 	printf("CollectionIterator Using inline storage %zu.\n", sizeof(ITPVector));
		// }

		for (; first != last; ++first){
			ITP itp{ *first, std::forward<Args>(args)... };

			if (itp)
				itp_.push_back(std::move(itp));
		}

		std::make_heap(std::begin(itp_), std::end(itp_), & CollectionIterator::heap_comp);
	}

	template<class StoreIterator>
	CollectionIterator(
			StoreIterator, StoreIterator,
			std::false_type
	){
		// skip the work and creates end iterator directly
		// printf("ITP = %zu\n", sizeof(ITP));
	}

public:
	reference operator*() const{
		return *itp_.front();
	}

	bool operator==(CollectionIterator const &other) const{
		return itp_.empty() && other.itp_.empty();
	}

	bool operator!=(CollectionIterator const &other) const{
		return ! operator==(other);
	}

	pointer operator ->() const{
		return & operator*();
	}

public:
	CollectionIterator &operator++(){
		increment_();

		return *this;
	}

private:
	void increment_(){
		while(!itp_.empty()){
			std::pop_heap(std::begin(itp_), std::end(itp_), & CollectionIterator::heap_comp);

			auto &ip = itp_.back();

			// ip is guearanteed to be valid
			auto const &key = ip->getKey();

			++ip;

			if (ip){
				std::push_heap(std::begin(itp_), std::end(itp_), & CollectionIterator::heap_comp);
			}else{
				itp_.pop_back();

				if (itp_.empty())
					return;
			}

			if ( key != itp_.front()->getKey() ){
				// pair keys not match
				return;
			}
		}
	}

	// heap compare
	static bool heap_comp(ITP const &a, ITP const &b){
		// returns ​true if the first argument is less than the second
		// however we negate this because we want min heap.
		return comp(a, b, std::true_type{}) > 0;
	}
};


} // namespace multi
} // namespace


//#include "collectioniterator.h.cc"


#endif

