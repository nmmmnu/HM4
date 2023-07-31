#include "vectorlist.h"

#include "mynarrow.h"
#include "binarysearch.h"

#define xfree(p)		free(p)
#define xrealloc(p, size)	realloc(p, (size_t) size)

#define xmemmove(d, s, size)	memmove(d, s, (size_t) size)


namespace hm4{


VectorList::VectorList(size_type const reallocCount) :
		reallocCount_( reallocCount ? reallocCount : 1 ) {
	clear_();
}

VectorList::VectorList(VectorList &&other):
		reallocCount_	(std::move(other.reallocCount_		)),
		buffer_		(std::move(other.buffer_		)),
		reservedCount_	(std::move(other.reservedCount_		)),
		dataCount_	(std::move(other.dataCount_		)),
		dataSize_	(std::move(other.dataSize_		)){
	other.clear_();
}

bool VectorList::clear(){
	for(auto it = buffer_; it < buffer_ + dataCount_; ++it)
		it->~OPair();

	clear_(true);

	return true;
}

inline auto VectorList::binarySearch_(const StringRef &key) const -> std::pair<bool,size_type>{
	assert(!key.empty());

	auto comp = [](const auto &list, auto const index, const auto &key){
		return list.cmpAt(index, key);
	};

	return binarySearch(*this, size_type{0}, size(), key, comp);
}

const Pair *VectorList::operator[](const StringRef &key) const{
	assert(!key.empty());

	const auto x = binarySearch_(key);

	return x.first ? operator[]( x.second ) : nullptr;
}

auto VectorList::lowerBound(const StringRef &key) const noexcept -> Iterator{
	if (key.empty())
		return buffer_;

	const auto x = binarySearch_(key);

	return buffer_ + x.second;
}

bool VectorList::insert(OPair&& newdata){
	assert(newdata);

	const StringRef &key = newdata->getKey();

	const auto x = binarySearch_(key);

	if (x.first){
		// key exists, overwrite, do not shift

		OPair &olddata = buffer_[ x.second ];

		// check if the data in database is valid
		if (! newdata->isValid(*olddata) ){
			// newdata will be magically destroyed.
			return false;
		}

		dataSize_ = dataSize_
					- olddata->bytes()
					+ newdata->bytes();

		// copy assignment
		olddata = std::move(newdata);

		return true;
	}

	// key not exists, shift, then add
	if ( ! shiftR_( x.second ) )
		return false;

	dataSize_ += newdata->bytes();

	// placement new with copy constructor
	void *placement = & buffer_[ x.second ];
	new(placement) OPair(std::move(newdata));

	return true;
}

bool VectorList::erase(const StringRef &key){
	assert(!key.empty());

	const auto x = binarySearch_(key);

	if (! x.first){
		// the key does not exists in the vector.
		return true;
	}

	// proceed with remove
	OPair &data = buffer_[x.second];
	dataSize_ -= data->bytes();
	data.~OPair();

	shiftL_(x.second);

	return true;
}

// ===================================

void VectorList::clear_(bool const alsoFree){
	if (alsoFree && buffer_)
		xfree(buffer_);

	dataCount_	= 0;
	dataSize_	= 0;
	reservedCount_	= 0;
	buffer_		= nullptr;
}

bool VectorList::shiftL_(size_type const index){
	// this is the most slow operation of them all
	xmemmove(
		& buffer_[index],
		& buffer_[index + 1],
		(dataCount_ - index - 1) * ELEMENT_SIZE
	);

	resize_(-1);

	return true;
}

bool VectorList::shiftR_(size_type const index){
	if (! resize_(1))
		return false;

	// LLVM static analyzer thinks this is an error,
	// so I add this unused branch
	if (buffer_){
		// this is the most slow operation of them all
		xmemmove(
			& buffer_[index + 1],
			& buffer_[index],
			(dataCount_ - index - 1) * ELEMENT_SIZE
		);
	}

	return true;
}

bool VectorList::resize_(int const delta){
	if (delta == 0){
		// already resized, done :)
		return true;
	}

	if (dataCount_ == 0 && delta < 0){
		// must be an error
		return true;
	}

	size_type const new_dataCount = dataCount_ + (size_type) sgn(delta);

	if (new_dataCount == 0){
		clear_(true);
		return true;
	}

	size_type const new_reservedCount = calcNewCount_(new_dataCount);

	if (reservedCount_ == new_reservedCount){
		// already resized, done :)
		dataCount_ = new_dataCount;

		return true;
	}

	OPair *new_buffer = (OPair *) xrealloc(buffer_, new_reservedCount * ELEMENT_SIZE);

	if (new_buffer == nullptr)
		return false;

	dataCount_	= new_dataCount;
	reservedCount_	= new_reservedCount;
	buffer_		= new_buffer;

	return true;
}

auto VectorList::calcNewCount_(size_type const count) const -> size_type{
	size_type newsize = count / reallocCount_;

	if (count % reallocCount_)
		++newsize;

	return newsize * reallocCount_;
}

} // namespace

