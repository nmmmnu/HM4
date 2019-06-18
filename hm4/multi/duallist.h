#ifndef DUAL_LIST_H_
#define DUAL_LIST_H_


#include "ilist.h"

#include "dualiterator.h"

namespace hm4{
namespace multi{


template <class List1, class List2, bool ERASE_WITH_TOMBSTONE = false>
class DualList{
public:
	using iterator		= DualIterator<
					typename List1::iterator,
					typename List2::iterator
				>;

	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

	using estimated_size	= std::true_type;

public:
	DualList(List1 &list1, const List2 &list2) :
					list1_(list1),
					list2_(list2){}

public:
	// Immutable Methods

	size_type size() const{
		// estimated
		return list1_.size() + list2_.size();
	}

	size_t bytes() const{
		return list1_.bytes() + list2_.bytes();
	}

public:
	iterator begin() const{
		return { list1_, list2_, std::true_type{} };
	}

	iterator end() const{
		return { list1_, list2_, std::false_type{} };
	}

	template <bool B>
	iterator find(StringRef const &key, std::bool_constant<B> const exact) const{
		return { list1_, list2_, key, exact };
	}

public:
	// Mutable Methods

	// wrong, but for compatibility
	bool clear(){
		return list1_.clear();
	}

	bool erase(StringRef const &key){
		assert(!key.empty());

		using tombstone_tag = std::bool_constant<ERASE_WITH_TOMBSTONE>;

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
	      List1	&list1_;
	const List2	&list2_;
};


} // multi

} // namespace


#endif
