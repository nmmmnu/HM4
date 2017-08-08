#ifndef DUAL_LIST_H_
#define DUAL_LIST_H_


#include "ilist.h"

#include "dualiterator.h"


namespace hm4{
namespace multi{


template <class LIST1, class LIST2=const LIST1, bool ERASE_WITH_TOMBSTONE=false>
class DualList : public IList<DualList<LIST1, LIST2, ERASE_WITH_TOMBSTONE>, LIST1::MUTABLE_TAG>{
public:
	using Iterator		= DualIterator<LIST1, LIST2>;

	using size_type		= typename DualList::size_type;

public:
//	DualList(std::nullptr_t){}

	DualList(LIST1 &list1, const LIST2 &list2) :
					list1_( & list1),
					list2_( & list2){
	}


public:
	// Immutable Methods

	ObserverPair operator[](const StringRef &key) const{
		assert(list1_ && list2_);
		assert(!key.empty());

		const Pair &pair = (*list1_)[key];

		if (pair)
			return Pair::observer(pair);

		return Pair::observer((*list2_)[key]);
	}

	size_type size(bool const estimated = false) const{
		assert(list1_ && list2_);

		return estimated ? sizeEstimated_(true) : sizeReal_();
	}

	size_t bytes() const{
		assert(list1_ && list2_);

		return list1_->bytes() + list2_->bytes();
	}

public:
	Iterator begin() const{
		assert(list1_ && list2_);

		return Iterator(*list1_, *list2_, typename Iterator::begin_iterator{} );
	}

	Iterator end() const{
		assert(list1_ && list2_);

		return Iterator(*list1_, *list2_, typename Iterator::end_iterator{} );
	}

	Iterator lowerBound(const StringRef &key) const{
		assert(list1_ && list2_);

		return Iterator(*list1_, *list2_, key );
	}

private:
	size_type sizeEstimated_(bool const estimated) const{
		return list1_->size(estimated) + list2_->size(estimated);
	}

	size_type sizeReal_() const{
		// Slooooow....
		size_type count = 0;
		for(auto it = begin(); it != end(); ++it)
			++count;

		return count;
	}

public:
	// Mutable Methods

	// wrong, but for compatibility
	bool clear(){
		assert(list1_);
		return list1_->clear();
	}

	bool erase(const StringRef &key){
		assert(list1_);
		assert(!key.empty());

		return erase_(key, erase_tag<ERASE_WITH_TOMBSTONE>{});
	}

private:
	template<bool T>
	struct erase_tag{};

	bool erase_(const StringRef &key, erase_tag<true>){
		return list1_->insert(Pair::tombstone(key));
	}

	bool erase_(const StringRef &key, erase_tag<false>){
		return list1_->erase(key);
	}

private:
	friend class IList<DualList<LIST1, LIST2, ERASE_WITH_TOMBSTONE>, true>;

	template <class UPAIR>
	bool insertT_(UPAIR &&data){
		assert(list1_);

		return list1_->insert( std::forward<UPAIR>(data) );
	}

private:
	      LIST1	*list1_ = nullptr;
	const LIST2	*list2_ = nullptr;
};


} // multi
} // namespace


#endif
