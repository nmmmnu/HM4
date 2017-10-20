#ifndef _VECTORLIST_H
#define _VECTORLIST_H

#include "ilist.h"

#include <cassert>

namespace hm4{


class VectorList : public IList<VectorList, true>{
	friend class IList;

public:
	static constexpr size_t		ELEMENT_SIZE		= sizeof(Pair);
	static constexpr size_type	DEFAULT_REALLOC_COUNT	= 16;

	class Iterator;

public:
	explicit
	VectorList(size_type reallocCount = DEFAULT_REALLOC_COUNT);
	VectorList(VectorList &&other);
	~VectorList(){
		clear();
	}

private:
	size_type	reallocCount_;

	OPair		*buffer_;
	size_type	reservedCount_;
	size_type	dataCount_;
	size_t		dataSize_;

public:
	bool clear();

	bool erase(const StringRef &key);

	const Pair *operator[](size_type const index) const{
		assert( index < size() );

		return buffer_[index].get();
	}

	int cmpAt(size_type index, const StringRef &key) const{
		assert( index < size() );
		assert(!key.empty());

		return operator[](index)->cmp(key);
	}

	bool insert(OPair &&data);

	size_type size(bool const = false) const{
		return dataCount_;
	}

	size_t bytes() const{
		return dataSize_;
	}

	const Pair *operator[](const StringRef &key) const;

public:
	Iterator lowerBound(const StringRef &key) const noexcept;

	Iterator begin() const noexcept;

	Iterator end() const noexcept;

private:
	void clear_(bool alsoFree = false);

	bool shiftL_(size_type index);
	bool shiftR_(size_type index);

	bool resize_(int delta);

	size_type calcNewCount_(size_type size);

private:
	/* preconditions
	Key can not be zero length
	*/
	std::pair<bool,size_type> binarySearch_(const StringRef &key) const;
};

// ==============================

class VectorList::Iterator{
private:
	friend class VectorList;

	constexpr Iterator(const OPair *pos) : pos_(pos){}

public:
	Iterator &operator++(){
		++pos_;
		return *this;
	}

	const Pair &operator*() const{
		return **pos_;
	}

public:
	bool operator==(const Iterator &other) const{
		return pos_ == other.pos_;
	}

	bool operator!=(const Iterator &other) const{
		return ! operator==(other);
	}

	const Pair *operator ->() const{
		return & operator*();
	}

private:
	const OPair	*pos_;
};

// ==============================

inline auto VectorList::begin() const noexcept -> Iterator{
	return buffer_;
}

inline auto VectorList::end() const noexcept -> Iterator{
	return buffer_ + dataCount_;
}


} // namespace


#endif
