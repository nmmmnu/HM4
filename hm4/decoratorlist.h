#ifndef DECORATOR_LIST_H_
#define DECORATOR_LIST_H_


#include "ilist.h"


namespace hm4{


template <class LIST>
class DecoratorList : public IList<DecoratorList<LIST>, LIST::MUTABLE_TAG>{
public:
	using Iterator		= typename LIST::Iterator;

	using size_type		= typename DecoratorList::size_type;

public:
	DecoratorList() = default;

	DecoratorList(LIST &list) : list_( & list){}


public:
	// Immutable Methods

	ObserverPair operator[](const StringRef &key) const{
		assert(list_);
		assert(!key.empty());
		return Pair::observer((*list_)[key]);
	}

	size_type size(bool const estimated = false) const{
		assert(list_);
		return list_->size(estimated);
	}

	size_t bytes() const{
		assert(list_);
		return list_->bytes();
	}

public:
	Iterator begin() const{
		assert(list_);
		return list_->begin();
	}

	Iterator end() const{
		assert(list_);
		return list_->end();
	}

	Iterator lowerBound(const StringRef &key) const{
		assert(list_);
		return list_->lowerBound(key);
	}

public:
	// Mutable Methods

	bool clear(){
		assert(list_);
		return list_->clear();
	}

	bool erase(const StringRef &key){
		assert(list_);
		assert(!key.empty());
		return list_->erase(key);
	}

private:
	friend class IList<DecoratorList<LIST>, LIST::MUTABLE_TAG>;

	template <class UPAIR>
	bool insertT_(UPAIR &&data){
		assert(list_);

		return list_->insert( std::forward<UPAIR>(data) );
	}

private:
	LIST	*list_ = nullptr;
};


} // namespace


#endif
