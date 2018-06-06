#ifndef _VECTORLIST_H
#define _VECTORLIST_H

#include "ilist.h"

#include "mynarrow.h"

#include <cassert>
#include <vector>

namespace hm4{


class VectorList : public IList<VectorList, true>{
	friend class IList;

	using OVector    = std::vector<OPair>;
	using OVectorIt  = VectorList::OVector::const_iterator;

public:
	class Iterator;

public:
	VectorList() = default;

	VectorList(VectorList &&other) :
				vector_		(std::move(other.vector_	)),
				dataSize_	(std::move(other.dataSize_	)){
		other.dataSize_ = 0;
	}

private:
	OVector		vector_;
	size_t		dataSize_ = 0;

public:
	bool clear(){
		vector_.clear();
		dataSize_ = 0;
		return true;
	}

	bool erase(const StringRef &key);

	const Pair *operator[](size_type const index) const{
		assert( index < size() );

		return vector_[index].get();
	}

	int cmpAt(size_type index, const StringRef &key) const{
		assert( index < size() );
		assert(!key.empty());

		const OPair &p = operator[](index);

		return p.cmp(key);
	}

	bool insert(OPair &&data);

	size_type size(bool const = false) const{
		return vector_.size();
	}

	size_t bytes() const{
		return dataSize_;
	}

	const Pair *operator[](const StringRef &key) const{
		assert(!key.empty());

		const auto x = binarySearch_(key);

		return x.first ? operator[]( x.second ) : nullptr;
	}

public:
	Iterator lowerBound(const StringRef &key) const noexcept;
	Iterator begin() const noexcept;
	Iterator end() const noexcept;

private:
	static OVectorIt beginOffset__(const OVector &vector, const std::pair<bool,size_type> &x){
		return vector.begin() + narrow<OVector::difference_type>(x.second);
	}

private:
	std::pair<bool,size_type> binarySearch_(const StringRef &key) const;
};

// ==============================

class VectorList::Iterator{
private:
	friend class VectorList;

	template<class UIT>
	constexpr Iterator(UIT &&it) : it_(std::forward<UIT>(it)){}

public:
	Iterator &operator++(){
		++it_;
		return *this;
	}

	const Pair &operator*() const{
		return **it_;
	}

public:
	bool operator==(const Iterator &other) const{
		return it_ == other.it_;
	}

	bool operator!=(const Iterator &other) const{
		return ! operator==(other);
	}

	const Pair *operator ->() const{
		return & operator*();
	}

private:
	OVectorIt	it_;
};

// ==============================

inline auto VectorList::begin() const noexcept -> Iterator{
	return vector_.begin();
}

inline auto VectorList::end() const noexcept -> Iterator{
	return vector_.end();
}

inline auto VectorList::lowerBound(const StringRef &key) const noexcept -> Iterator{
	if (key.empty())
		return begin();

	const auto x = binarySearch_(key);

	return beginOffset__(vector_, x);
}

} // namespace


#endif
