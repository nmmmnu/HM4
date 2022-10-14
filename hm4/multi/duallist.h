#ifndef DUAL_LIST_H_
#define DUAL_LIST_H_

#include "ilist.h"

#include "dualiterator.h"
#include <cassert>
#include <type_traits>

namespace hm4{
namespace multi{

enum class DualListEraseType{
	NORMAL,
	TOMBSTONE
};

template <class List1, class List2, DualListEraseType EraseType>
class DualListBase{
public:
	using iterator		= DualIterator<
					typename List1::iterator,
					typename List2::iterator
				>;

	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

	using estimated_size	= std::true_type;

public:
	DualListBase(List1 &list1, List2 &list2) :
					list1_(&list1),
					list2_(&list2){}

public:
	// Immutable Methods

	size_type size() const{
		// estimated
		return list1_->size() + list2_->size();
	}

	auto mutable_size() const{
		return list1_->mutable_size();
	}

	constexpr void mutable_notify(const Pair *p, PairFactoryMutableNotifyMessage const &message){
		return list1_->mutable_notify(p, message);
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

protected:
	List1	*list1_;
	List2	*list2_;
};



template<class List1, class List2, DualListEraseType EraseType, class = std::void_t<> >
class DualList : public DualListBase<List1, List2, EraseType>{
public:
	using DualListBase<List1, List2, EraseType>::DualListBase;
};



template<class List1, class List2, DualListEraseType EraseType>
class DualList<List1, List2, EraseType, std::void_t<typename List1::Allocator> > : public DualListBase<List1, List2, EraseType>{
public:
	using Base_ = DualListBase<List1, List2, EraseType>;
	using iterator  = typename Base_::iterator;

	using Allocator = typename List1::Allocator;

	const auto &getAllocator() const{
		return list1_->getAllocator();
	}

	auto &getAllocator(){
		return list1_->getAllocator();
	}

	using Base_::DualListBase;

public:
	// Mutable Methods

	// wrong, but for compatibility
	bool clear(){
		return list1_->clear();
	}

	bool erase_(std::string_view const key){
		assert(Pair::check(key));

		if constexpr (EraseType == DualListEraseType::NORMAL){
			return hm4::erase(*list1_, key);
		}

		if constexpr (EraseType == DualListEraseType::TOMBSTONE){
			return hm4::insert(*list1_, key) != std::end(*list1_);
		}
	}

	template<class PFactory>
	auto insertLazyPair_(PFactory &&factory){
		return fixDualIterator_(
			list1_->insertLazyPair_(std::move(factory))
		);
	}

protected:
	iterator fixDualIterator_(typename List1::iterator &&it){
		return {
			{	std::move(it),		std::end(*list1_)	},
			{	std::end(*list2_),	std::end(*list2_)	}
		};
	}

protected:
	using Base_::list1_;
	using Base_::list2_;
};



} // multi
} // namespace



#endif
