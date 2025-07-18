#ifndef DUAL_LIST_H_
#define DUAL_LIST_H_

#include "ilist.h"

#include "dualiterator.h"
#include <cassert>
#include <type_traits>

namespace hm4::multi{

enum class DualListEraseType{
	NORMAL		,
	TOMBSTONE	,
	SMART_TOMBSTONE	,
	NONE
};

template <class List1, class List2>
class DualListBase{
public:
	using iterator		= DualIterator<
					typename List1::iterator,
					typename List2::iterator
				>;

	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

public:
	constexpr static bool conf_estimated_size	= true;

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

	auto empty() const{
		// unlike CollectionList,
		// if list2_->empty() is constexpr,
		// the optimizer will remove it

		return list1_->empty() && list2_->empty();
	}

	auto const &mutable_list() const{
		return list1_->mutable_list();
	}

	constexpr uint64_t mutable_version() const{
		return list1_->mutable_version();
	}

	constexpr void mutable_notify( PairFactoryMutableNotifyMessage const &message){
		return list1_->mutable_notify(message);
	}

	size_t bytes() const{
		return list1_->bytes() + list2_->bytes();
	}

	constexpr void crontab(){
		list1_->crontab();
		list2_->crontab();
	}

	constexpr void crontab() const{
		list1_->crontab();
		list2_->crontab();
	}

public:
	iterator begin() const{
		return { *list1_, *list2_, std::true_type{} };
	}

	iterator end() const{
		return { *list1_, *list2_, std::false_type{} };
	}

	iterator find(std::string_view const key) const{
		return { *list1_, *list2_, key };
	}

	const Pair *getPair(std::string_view const key) const{
		const auto *p1 = list1_->getPair(key);
		const auto *p2 = list2_->getPair(key);

		if (!p1) return p2;
		if (!p2) return p1;

		return p1->cmpTime(*p2) > 0 ? p1 : p2;
	}

protected:
	List1	*list1_;
	List2	*list2_;
};



template<class List1, class List2, DualListEraseType = DualListEraseType::NONE, class = std::void_t<> >
struct DualList : public DualListBase<List1, List2>{
	using DualListBase<List1, List2>::DualListBase;
};



template<class List1, class List2, DualListEraseType EraseType>
class DualList<List1, List2, EraseType, std::void_t<typename List1::Allocator> > : public DualListBase<List1, List2>{
	static_assert(EraseType != DualListEraseType::NONE);
public:
	using Base_ = DualListBase<List1, List2>;
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

	InsertResult erase_(std::string_view const key){
		assert(!key.empty());

		// will never come here.
		if constexpr (EraseType == DualListEraseType::NONE)
			return InsertResult::skipDeleted();

		if constexpr (EraseType == DualListEraseType::NORMAL)
			return hm4::erase(*list1_, key);

		if constexpr (EraseType == DualListEraseType::TOMBSTONE)
			return hm4::insertTS(*list1_, key);

		if constexpr (EraseType == DualListEraseType::SMART_TOMBSTONE){
			if (list2_->empty()){
				logger<Logger::DEBUG>() << "SMART_TOMBSTONE: erase" << key;
				return hm4::erase(*list1_, key);
			}else{
				logger<Logger::DEBUG>() << "SMART_TOMBSTONE: insertTS" << key;
				return hm4::insertTS(*list1_, key);
			}
		}
	}

	template<class PFactory>
	InsertResult insertF(PFactory &factory){
		return list1_->insertF(factory);
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



} // namespace



#endif
