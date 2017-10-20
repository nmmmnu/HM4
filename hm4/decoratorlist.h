#ifndef DECORATOR_LIST_H_
#define DECORATOR_LIST_H_


#include "ilist.h"


namespace hm4{


template <class LIST, class CRPT_USER>
class DecoratorList : public IList<CRPT_USER, LIST::MUTABLE>{
public:
	using Iterator		= typename LIST::Iterator;

	using size_type		= typename DecoratorList::size_type;

protected:
	DecoratorList(LIST &list) : list_(list){}


public:
	// Immutable Methods

	const Pair *operator[](const StringRef &key) const{
		assert(!key.empty());
		return list_[key];
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

	bool insert(OPair &&data){
		return list_.insert( std::move(data) );
	}

private:
	LIST	&list_;
};


template <class LIST>
struct PureDecoratorList : public DecoratorList<LIST, PureDecoratorList<LIST> >{
	PureDecoratorList(LIST &list) : DecoratorList<LIST, PureDecoratorList<LIST> >(list){}
};


} // namespace


#endif
