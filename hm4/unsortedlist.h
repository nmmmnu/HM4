#ifndef UNSORTED_LIST_H_
#define UNSORTED_LIST_H_

#include "ilist.h"
#include "listcounter.h"

#include "mynarrow.h"

#include <vector>
#include <algorithm>

#include "pmallocator.h"

namespace hm4{



template<class T_Allocator>
class UnsortedList{
	using OVector	= std::vector<Pair *>;
	using OVectorIt	= OVector::const_iterator;

public:
	using Allocator		= T_Allocator;
	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

public:
	class iterator;

public:
	UnsortedList(Allocator &allocator) : allocator_(& allocator){}

	UnsortedList(UnsortedList &&other) = default;

	~UnsortedList(){
		clear();
	}

	void prepareFlush(){
		if (needSort_ == false)
			return;

		auto comp = [](const Pair *p1, const Pair *p2){
			// return bigger time or first
			return p1->cmpWithTime(*p2, std::false_type{}) < 0;
		};

		std::sort(std::begin(vector_), std::end(vector_), comp);

		// this keep first occurance
		vector_.erase(
			std::unique(std::begin(vector_), std::end(vector_), equals),
			std::end(vector_)
		);

		needSort_ = false;
	}

private:
	OVector		vector_;
	ListCounter	lc_;
	bool		needSort_ = false;
	Allocator	*allocator_;

public:
	bool clear(){
		if (allocator_->reset() == false){
			std::for_each(std::begin(vector_), std::end(vector_), [this](void *p){
				using namespace MyAllocator;
				deallocate(allocator_, p);
			});
		}

		vector_.clear();
		lc_.clr();
		return true;
	}

	template<class PFactory>
	iterator insertLazyPair_(PFactory &&factory);

	auto size() const{
		return lc_.size();
	}

	auto mutable_size() const{
		return size();
	}

	constexpr static void mutable_notify(const Pair *, PairFactoryMutableNotifyMessage const &){
	}

	auto bytes() const{
		return lc_.bytes();
	}

	const Allocator &getAllocator() const{
		return *allocator_;
	}

	Allocator &getAllocator(){
		return *allocator_;
	}

public:
	iterator begin() const noexcept;
	iterator end() const noexcept;
};

// ==============================

template<class T_Allocator>
class UnsortedList<T_Allocator>::iterator{
public:
	iterator(OVectorIt const &it) : ptr(it){}

public:
	using difference_type = UnsortedList::difference_type;
	using value_type = const Pair;
	using pointer = value_type *;
	using reference = value_type &;
	using iterator_category = std::forward_iterator_tag;

public:
	iterator &operator++(){
		++ptr;
		return *this;
	}

	reference operator*() const{
		return **ptr;
	}

public:
	bool operator==(const iterator &other) const{
		return ptr == other.ptr;
	}

	bool operator!=(const iterator &other) const{
		return ! operator==(other);
	}

	pointer operator ->() const{
		return & operator*();
	}

private:
	OVectorIt	ptr;
};

// ==============================

template<class T_Allocator>
inline auto UnsortedList<T_Allocator>::begin() const noexcept -> iterator{
	return std::begin(vector_);
}

template<class T_Allocator>
inline auto UnsortedList<T_Allocator>::end() const noexcept -> iterator{
	return std::end(vector_);
}

template<class T_Allocator>
template<class PFactory>
inline auto UnsortedList<T_Allocator>::insertLazyPair_(PFactory &&factory) -> iterator{
	auto newdata = factory(getAllocator());

	if (!newdata)
		return this->end();

	try{
		vector_.push_back(newdata.get());

		lc_.inc(newdata->bytes());

		newdata.release();

		needSort_ = true;

		return { std::prev(std::end(vector_)) };
	}catch(...){
		// newdata will be deallocated...
		return this->end();
	}
}



} // namespace


#endif
