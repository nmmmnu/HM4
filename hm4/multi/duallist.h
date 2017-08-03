#ifndef DUAL_LIST_H_
#define DUAL_LIST_H_


#include "dualtable.h"


namespace hm4{
namespace multi{


template <class LIST, class TABLE, bool ERASE_WITH_TOMBSTONE>
class DualList : public IMutableList<DualList<LIST,TABLE,ERASE_WITH_TOMBSTONE> >{
private:
	using MyDualTable	= DualTable<LIST,TABLE>;

public:
	using Iterator		= typename MyDualTable::Iterator;

	using size_type		= typename DualList::size_type;

public:
	DualList() = default;

	DualList(LIST &list, const TABLE &table) :
					dtable_(list, table),
					list_( & list){
	}

// OWN METHODS

public:
	// wrong, but for compatibility
	bool clear(){
		assert(list_);
		return list_->clear();
	}

	bool erase(const StringRef &key){
		assert(list_);
		assert(!key.empty());

		return erase_(key, erase_tag<ERASE_WITH_TOMBSTONE>{});
	}

private:
	template<bool T>
	struct erase_tag{};

	bool erase_(const StringRef &key, erase_tag<true>){
		return list_->insert(Pair::tombstone(key));
	}

	bool erase_(const StringRef &key, erase_tag<false>){
		return list_->erase(key);
	}

private:
	friend class IMutableList<DualList<LIST,TABLE,ERASE_WITH_TOMBSTONE> >;

	template <class UPAIR>
	bool insertT_(UPAIR &&data){
		assert(list_);

		return list_->insert( std::forward<UPAIR>(data) );
	}

// DELEGATED METHODS

public:
	Pair operator[](const StringRef &key) const{
		assert(!key.empty());
		return dtable_[key];
	}

	size_type size(bool const estimated = false) const{
		return dtable_.size(estimated);
	}

	size_t bytes() const{
		return dtable_.bytes();
	}

public:
	Iterator begin() const{
		return dtable_.begin();
	}

	Iterator end() const{
		return dtable_.end();
	}

	Iterator lowerBound(const StringRef &key) const{
		return dtable_.lowerBound(key);
	}

private:
	MyDualTable	dtable_;
	LIST		*list_ = nullptr;
};


} // multi
} // namespace


#endif
