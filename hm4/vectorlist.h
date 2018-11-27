#ifndef _VECTORLIST_H
#define _VECTORLIST_H

#include "ilist.h"

#include "mynarrow.h"
#include "binarysearch.h"

#include <cassert>
#include <vector>

namespace hm4{


class VectorList{
	using OVector	= std::vector<OPair>;
	using OVectorIt	= OVector::const_iterator;

public:
	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

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

	size_type size() const{
		return vector_.size();
	}

	size_type sizeEstimated() const{
		return size();
	}

	size_t bytes() const{
		return dataSize_;
	}

	Pair const *operator[](StringRef const &key) const{
		assert(!key.empty());

		auto const x = binarySearch_(key);

		return x.found ? operator[]( x.pos ) : nullptr;
	}

public:
	Iterator lowerBound(StringRef const &key) const noexcept;
	Iterator begin() const noexcept;
	Iterator end() const noexcept;

private:
	static OVectorIt beginOffset__(OVector const &vector, size_type const pos){
		return vector.begin() + narrow<OVector::difference_type>(pos);
	}

private:
	BinarySearchResult<size_type> binarySearch_(StringRef const &key) const;
};

// ==============================

class VectorList::Iterator{
private:
	friend class VectorList;
	constexpr Iterator(OVectorIt const &it) : it_(it){}
	constexpr Iterator(OVectorIt &&it) : it_(std::move(it)){}

public:
	constexpr Iterator(Iterator const &other) = default;
	constexpr Iterator(Iterator &&other) = default;

public:
	using difference_type = VectorList::difference_type;
	using value_type = Pair;
	using pointer = const value_type *;
	using reference = value_type &;
	using iterator_category = std::bidirectional_iterator_tag;

public:
	Iterator &operator++(){
		++it_;
		return *this;
	}

	Iterator &operator--(){
		--it_;
		return *this;
	}

public:
	bool operator==(Iterator const &other) const{
		return it_ == other.it_;
	}

	bool operator!=(Iterator const &other) const{
		return it_ != other.it_;
	}

	bool operator > (Iterator const &other) const{
		return operator>(other);
	}

	bool operator >= (Iterator const &other) const{
		return operator>=(other);
	}

	bool operator < (Iterator const &other) const{
		return operator<(other);
	}

	bool operator <= (Iterator const &other) const{
		return operator<=(other);
	}

public:
	const Pair &operator*() const{
		return **it_;
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

	return beginOffset__(vector_, x.pos);
}

} // namespace


#endif
