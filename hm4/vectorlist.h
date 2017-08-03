#ifndef _VECTORLIST_H
#define _VECTORLIST_H

#include "ilist.h"

#include <cassert>

namespace hm4{


class VectorList : public IMutableList<VectorList>{
public:
	static constexpr size_t		ELEMENT_SIZE		= sizeof(Pair);
	static constexpr size_type	DEFAULT_REALLOC_COUNT	= 16;

	using Iterator = const Pair *;

public:
	explicit
	VectorList(size_type reallocCount = DEFAULT_REALLOC_COUNT);
	VectorList(VectorList &&other);
	~VectorList(){
		clear();
	}

private:
	size_type	reallocCount_;

	Pair		*buffer_;
	size_type	reservedCount_;
	size_type	dataCount_;
	size_t		dataSize_;

public:
	bool clear();

	/* preconditions
	Key can not be zero length
	*/
	bool erase(const StringRef &key);

	const Pair &operator[](size_type const index) const{
		// precondition
		assert( index < size() );
		// eo precondition

		return buffer_[index];
	}

	int cmpAt(size_type const index, const StringRef &key) const{
		// precondition
		assert( index < size() );
		assert( ! key.empty() );
		// eo precondition

		return operator[](index).cmp(key);
	}

	size_type size(bool const = false) const{
		return dataCount_;
	}

	size_t bytes() const{
		return dataSize_;
	}

	/* preconditions
	Key can not be zero length
	*/
	const Pair &operator[](const StringRef &key) const;

private:
	friend class IMutableList<VectorList>;

	template <class UPAIR>
	bool insertT_(UPAIR &&data);

public:
	Iterator lowerBound(const StringRef &key) const noexcept;

	Iterator begin() const noexcept{
		return buffer_;
	}

	Iterator end() const noexcept{
		return buffer_ + dataCount_;
	}

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

} // namespace


#endif
