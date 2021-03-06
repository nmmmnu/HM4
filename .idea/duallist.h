#ifndef DUAL_LIST_H_
#define DUAL_LIST_H_

#include "ilist.h"

#include "dualiterator.h"
#include <cassert>

namespace hm4{
namespace multi{


template <class List1, class List2, bool EraseWithTombstone>
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
					list1_(&list1),
					list2_(&list2){}

public:
	// Immutable Methods

	size_type size() const{
		// estimated
		return list1_->size() + list2_->size();
	}

	size_t bytes() const{
		return list1_->bytes() + list2_->bytes();
	}

public:
	iterator begin() const{
		return { *list1_, *list2_, std::true_type{} };
	}

	iterator end() const{
		return { *list1_, *list2_, std::false_type{} };
	}

	template <bool B>
	iterator find(std::string_view const key, std::bool_constant<B> const exact) const{
		return { *list1_, *list2_, key, exact };
	}

public:
	// Mutable Methods

	// wrong, but for compatibility
	bool clear(){
		return list1_->clear();
	}

	bool erase(std::string_view const key){
		assert(Pair::check(key));

		if constexpr (EraseWithTombstone){
			return list1_->insert(key, Pair::TOMBSTONE) != std::end(*list1_);
		}else{
			return list1_->erase(key);
		}
	}

	iterator insert(	std::string_view const key, std::string_view const val,
			uint32_t const expires = 0, uint32_t const created = 0
			){

		return insert_( hm4::insert(*list1_, key, val, expires, created) );
	}

	iterator insert(Pair const &src){
		return insert_( hm4::insert(*list1_, src) );
	}

private:
	iterator insert_(typename List1::iterator &&it){
		return {
			{	std::move(it),		std::end(*list1_)	},
			{	std::end(*list2_),	std::end(*list2_)	}
		};
	}

private:
	      List1	*list1_;
	const List2	*list2_;
};



template <class List, bool EraseWithTombstone>
class DualListSame : public DualList<List, List, EraseWithTombstone>{
public:
	using Allocator		= typename List::Allocator;

	DualListSame(List &list1, const List &list2) :
					DualList<List, List, EraseWithTombstone>(list1, list2),
					list_(&list1){}

	Allocator &getAllocator() const{
		return list_->getAllocator();
	}

private:
	const List	*list_;

};



} // multi

} // namespace


#endif
