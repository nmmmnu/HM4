#include "vectorlist.h"

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
//	for(auto &p : ArrayRef<Pair>{ buffer_, dataCount_ } )
	for(auto it = buffer_; it < buffer_ + dataCount_; ++it)
		it->~Pair();

	clear_(true);

	return true;
}

inline auto VectorList::binarySearch_(const StringRef &key) const -> std::pair<bool,size_type>{
	// precondition
	assert(!key.empty());
	// eo precondition

	return binarySearch(*this, size_type(0), size(), key, BinarySearchCompList{});
}

const Pair &VectorList::operator[](const StringRef &key) const{
	// precondition
	assert(!key.empty());
	// eo precondition

	const auto x = binarySearch_(key);

	return x.first ? operator[]( x.second ) : Pair::zero();
}

auto VectorList::lowerBound(const StringRef &key) const noexcept -> Iterator{
	if (key.empty())
		return buffer_;

	const auto x = binarySearch_(key);

	return buffer_ + x.second;
}

template <class UPAIR>
bool VectorList::insertT_(UPAIR&& newdata){
	assert(newdata);

	const StringRef &key = newdata.getKey();

	const auto x = binarySearch_(key);

	if (x.first){
		// key exists, overwrite, do not shift

		Pair & olddata = buffer_[ x.second ];

		// check if the data in database is valid
		if (! newdata.isValid(olddata) ){
			// newdata will be magically destroyed.
			return false;
		}

		dataSize_ = dataSize_
					- olddata.bytes()
					+ newdata.bytes();

		// copy assignment
		olddata = std::forward<UPAIR>(newdata);

		return true;
	}

	// key not exists, shift, then add
	if ( ! shiftR_( x.second ) )
		return false;

	dataSize_ += newdata.bytes();

	// placement new with copy constructor
	void *placement = & buffer_[ x.second ];
	new(placement) Pair(std::forward<UPAIR>(newdata));

	return true;
}

bool VectorList::erase(const StringRef &key){
	// precondition
	assert(!key.empty());
	// eo precondition

	const auto x = binarySearch_(key);

	if (! x.first){
		// the key does not exists in the vector.
		return true;
	}

	// proceed with remove
	Pair & data = buffer_[x.second];
	dataSize_ -= data.bytes();
	data.~Pair();

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
		size_t const size = dataCount_ - index - 1;

		// this is the most slow operation of them all
		xmemmove(
			& buffer_[index + 1],
			& buffer_[index],
			size * ELEMENT_SIZE
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

	Pair *new_buffer = (Pair *) xrealloc(buffer_, new_reservedCount * ELEMENT_SIZE);

	if (new_buffer == nullptr)
		return false;

	dataCount_	= new_dataCount;
	reservedCount_	= new_reservedCount;
	buffer_		= new_buffer;

	return true;
}

auto VectorList::calcNewCount_(size_type const count) -> size_type{
	size_type newsize = count / reallocCount_;

	if (count % reallocCount_)
		++newsize;

	return newsize * reallocCount_;
}

// ===================================

template bool VectorList::insertT_(Pair &&newdata);
template bool VectorList::insertT_(const Pair &newdata);

} // namespace

