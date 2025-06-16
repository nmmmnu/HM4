#ifndef _COLLECTION_MULTI_TABLE_ITERATOR_H
#define _COLLECTION_MULTI_TABLE_ITERATOR_H

#include "iteratorpair.h"

#include <vector>
#include <algorithm>	// heap


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

	using ITPVector		= std::vector<ITP>;

	ITPVector		itp_;

public:
	template<class StoreIterator, typename ...Args>
	CollectionIterator(
			StoreIterator first, StoreIterator last,
			Args&& ...args
	){
		itp_.reserve( static_cast<typename ITPVector::size_type>( std::distance(first, last) ) );

		for (; first != last; ++first){
			ITP itp{ *first, std::forward<Args>(args)... };

			if (itp)
				itp_.push_back(std::move(itp));
		}

		make_heap(std::begin(itp_), std::end(itp_), & CollectionIterator::heap_comp);
	}

	template<class StoreIterator>
	CollectionIterator(
			StoreIterator, StoreIterator,
			std::false_type
	){
		// skip the work and creates end iterator directly
		// printf("Here...\n");
	}

#if 0
	template<class StoreIterator>
	CollectionIterator(
			StoreIterator first, StoreIterator last,
			std::string_view const key,
			std::true_type
	){
		// skip the work and creates iterator with single element

		// this is std::min_element,
		// so it can not use the algorithm,
		// from the other side, result needed is the projection,
		// so it can not be written as a template...

		if (first == last){
			// not found. done.
			return;
		}

		auto proj = [key](auto const &table){
			return ITP{ table, key, std::true_type{} };
		};

		auto min_element = proj(*first);

		++first;

		for(; first != last; ++first){
			auto element = proj(*first);

			if (heap_comp(min_element, element))
				min_element = std::move(element);
		}

		if (min_element){
			itp_.reserve(1);
			itp_.push_back(std::move(min_element));
		}
	}
#endif

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
			pop_heap(std::begin(itp_), std::end(itp_), & CollectionIterator::heap_comp);

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

