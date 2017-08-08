#ifndef DECORATOR_LIST_H_
#define DECORATOR_LIST_H_


#include "ilist.h"


namespace hm4{


template <class LIST, class T>
class DecoratorList : public IList<T, LIST::MUTABLE_TAG>{
public:
	using Iterator		= typename LIST::Iterator;

	using size_type		= typename DecoratorList::size_type;

protected:
	DecoratorList(LIST &list) : list_(list){}


public:
	// Immutable Methods

	ObserverPair operator[](const StringRef &key) const{
		assert(!key.empty());
		return Pair::observer(list_[key]);
	}

	size_type size(bool const estimated = false) const{
		return list_.size(estimated);
	}

	size_t bytes() const{
		return list_.bytes();
	}

public:
	Iterator begin() const{
		return list_.begin();
	}

	Iterator end() const{
		return list_.end();
	}

	Iterator lowerBound(const StringRef &key) const{
		return list_.lowerBound(key);
	}

public:
	// Mutable Methods

	bool clear(){
		return list_.clear();
	}

	bool erase(const StringRef &key){
		assert(!key.empty());
		return list_.erase(key);
	}

private:
	friend class IList<T, LIST::MUTABLE_TAG>;

	template <class UPAIR>
	bool insertT_(UPAIR &&data){
		return list_.insert( std::forward<UPAIR>(data) );
	}

private:
	LIST	&list_;
};


template <class LIST>
struct PureDecoratorList : public DecoratorList<LIST, PureDecoratorList<LIST> >{
//	PureDecoratorList() = default;
	PureDecoratorList(LIST &list) : DecoratorList<LIST, PureDecoratorList<LIST> >(list){}
};


} // namespace


#endif
