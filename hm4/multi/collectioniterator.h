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
		reserve__(first, last, itp_);

		for (; first != last; ++first){
			auto const &list = *first;

			ITP itp{ list, std::forward<Args>(args)... };

			if (itp)
				itp_.push_back(std::move(itp));
		}

		make_heap(itp_);
	}

	template<class StoreIterator>
	constexpr CollectionIterator(
			StoreIterator const &, StoreIterator const &,
			std::false_type
	){
		// skip the work and creates end iterator directly
		// printf("Here...\n");
	}

	template<class StoreIterator>
	CollectionIterator(
			StoreIterator first, StoreIterator last,
			StringRef const &key,
			std::true_type const exact
	){
		// skip the work and creates iterator with single element
		// printf("Here...\n");

		if (first == last){
			// not found. done.
			return;
		}

		ITP smallest{ first->end(), first->end() };

		for(; first != last; ++first){
			auto const &list = *first;

			ITP itp{ list, key, exact };

			if (heap_comp(smallest, itp))
				smallest = std::move(itp);
		}

		if (smallest){
			itp_.reserve(1);
			itp_.push_back(std::move(smallest));
		}
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
		const Pair *p = nullptr;

		while(!itp_.empty()){
			// ip is guearanteed to be valid

			if (p && p->cmp(itp_.front()->getKey()) != 0){
				// pair keys not match
				return;
			}

			pop_heap(itp_);

			auto &ip = itp_.back();

			if (p == nullptr){
				// first pass
				p = ip.ptr();
			}

			++ip;

			if (ip){
				push_heap(itp_);
			}else{
				itp_.pop_back();
			}
		}
	}

	template<class StoreIterator>
	static void reserve__(StoreIterator first, StoreIterator last, ITPVector &v){
		auto const size = std::distance(first, last);
		if (size > 0)
			v.reserve( static_cast<typename ITPVector::size_type>(size) );
	}

private:
	// heap compare
	static bool heap_comp(ITP const &a, ITP const &b){
		// returns ​true if the first argument is less than the second
		// however we negate this because we want min heap.
		return multiiterator_impl_::comp(a, b) > 0;
	}

	// heap functions
	static auto make_heap(ITPVector &v){
		return std::make_heap(std::begin(v), std::end(v), & CollectionIterator::heap_comp);
	}

	static auto push_heap(ITPVector &v){
		return std::push_heap(std::begin(v), std::end(v), & CollectionIterator::heap_comp);
	}

	static auto pop_heap(ITPVector &v){
		return std::pop_heap(std::begin(v), std::end(v), & CollectionIterator::heap_comp);
	}
};


} // namespace multi
} // namespace


//#include "collectioniterator.h.cc"


#endif

