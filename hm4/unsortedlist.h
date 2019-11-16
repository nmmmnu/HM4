#ifndef UNSORTED_LIST_H_
#define UNSORTED_LIST_H_

#include "ilist.h"
#include "listcounter.h"

#include "mynarrow.h"

#include <vector>
#include <algorithm>

#include "pmallocator.h"

namespace hm4{


class UnsortedList{
	using OVector	= std::vector<Pair *>;
	using OVectorIt	= OVector::const_iterator;
	using Allocator	= MyAllocator::PMAllocator;

public:
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

	void sort(){
		if (needSort_ == false)
			return;

		std::sort(std::begin(vector_), std::end(vector_), less);

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
				allocator_->deallocate(p);
			});
		}

		vector_.clear();
		lc_.clr();
		return true;
	}

	bool insert(	std::string_view key, std::string_view val,
			uint32_t expires = 0, uint32_t created = 0){

		auto newdata = Pair::up::create(*allocator_, key, val, expires, created);

		if (!newdata)
			return false;

		try{
			vector_.push_back(newdata.get());
			lc_.inc(newdata->bytes());
		}catch(...){
			return false;
		}

		newdata.release();

		needSort_ = true;

		return true;
	}

	auto size() const{
		return lc_.size();
	}

	auto bytes() const{
		return lc_.bytes();
	}

	Allocator const &getAllocator() const{
		return *allocator_;
	}

public:
	iterator begin() noexcept;
	iterator end() const noexcept;
};

// ==============================

class UnsortedList::iterator{
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

inline auto UnsortedList::begin() noexcept -> iterator{
	sort();

	return std::cbegin(vector_);
}

inline auto UnsortedList::end() const noexcept -> iterator{
	return std::end(vector_);
}

} // namespace


#endif
