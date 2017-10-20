#ifndef DUAL_LIST_H_
#define DUAL_LIST_H_


#include "ilist.h"

#include "dualiterator.h"

#include <type_traits>


namespace hm4{
namespace multi{


template <class LIST1, class LIST2=const LIST1, bool ERASE_WITH_TOMBSTONE=false>
class DualList : public IList<DualList<LIST1, LIST2, ERASE_WITH_TOMBSTONE>, LIST1::MUTABLE>{
public:
	using Iterator		= DualIterator<LIST1, LIST2>;

	using size_type		= typename DualList::size_type;

public:
	DualList(LIST1 &list1, const LIST2 &list2) :
					list1_(list1),
					list2_(list2){
	}


public:
	// Immutable Methods

	const Pair *operator[](const StringRef &key) const{
		assert(!key.empty());

		const Pair *pair = list1_[key];

		if (pair)
			return pair;

		return list2_[key];
	}

	size_type size(bool const estimated = false) const{
		return estimated ? sizeEstimated_(true) : this->sizeViaIterator_();
	}

	size_t bytes() const{
		return list1_.bytes() + list2_.bytes();
	}

public:
	Iterator begin() const{
		return Iterator(list1_, list2_, typename Iterator::begin_iterator{} );
	}

	Iterator end() const{
		return Iterator(list1_, list2_, typename Iterator::end_iterator{} );
	}

	Iterator lowerBound(const StringRef &key) const{
		return Iterator(list1_, list2_, key );
	}

private:
	size_type sizeEstimated_(bool const estimated) const{
		return list1_.size(estimated) + list2_.size(estimated);
	}

public:
	// Mutable Methods

	// wrong, but for compatibility
	bool clear(){
		return list1_.clear();
	}

	bool erase(const StringRef &key){
		assert(!key.empty());

		return erase_(key, std::integral_constant<bool, ERASE_WITH_TOMBSTONE>{});
	}

	bool insert(OPair &&data){
		return list1_.insert( std::move(data) );
	}

private:
	bool erase_(const StringRef &key, std::true_type){
		return list1_.insert(OPair::tombstone(key));
	}

	bool erase_(const StringRef &key, std::false_type){
		return list1_.erase(key);
	}

private:
	      LIST1	&list1_;
	const LIST2	&list2_;
};


} // multi
} // namespace


#endif
