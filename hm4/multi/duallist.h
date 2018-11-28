#ifndef DUAL_LIST_H_
#define DUAL_LIST_H_


#include "ilist.h"

#include "dualiterator.h"

#include <type_traits>


namespace hm4{
namespace multi{


template <class LIST1, class LIST2=const LIST1, bool ERASE_WITH_TOMBSTONE=false>
class DualList{
public:
	using Iterator		= DualIterator<LIST1, LIST2>;

	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

	using estimated_size	= std::true_type;

public:
	DualList(LIST1 &list1, const LIST2 &list2) :
					list1_(list1),
					list2_(list2){}

public:
	// Immutable Methods

	const Pair *operator[](StringRef const &key) const{
		assert(!key.empty());

		const Pair *pair = list1_[key];

		if (pair)
			return pair;

		return list2_[key];
	}

	size_type size() const{
		// estimated
		return list1_.size() + list2_.size();
	}

	size_t bytes() const{
		return list1_.bytes() + list2_.bytes();
	}

public:
	Iterator begin() const{
		return { list1_, list2_, std::true_type{} };
	}

	Iterator end() const{
		return { list1_, list2_, std::false_type{} };
	}

	Iterator lowerBound(const StringRef &key) const{
		return { list1_, list2_, key };
	}

public:
	// Mutable Methods

	// wrong, but for compatibility
	bool clear(){
		return list1_.clear();
	}

	bool erase(StringRef const &key){
		assert(!key.empty());

		using tombstone_tag = std::integral_constant<bool, ERASE_WITH_TOMBSTONE>;

		return erase_(key, tombstone_tag{});
	}

	bool insert(OPair &&data){
		return list1_.insert( std::move(data) );
	}

private:
	bool erase_(StringRef const &key, std::true_type){
		return list1_.insert(OPair::tombstone(key));
	}

	bool erase_(StringRef const &key, std::false_type){
		return list1_.erase(key);
	}

private:
	      LIST1	&list1_;
	const LIST2	&list2_;
};


} // multi

} // namespace


#endif
