#ifndef DECORATOR_LIST_H_
#define DECORATOR_LIST_H_


#include "ilist.h"


namespace hm4{


template <class LIST>
class DecoratorList{
public:
	using Iterator		= typename LIST::Iterator;

	using size_type		= typename LIST::size_type;
	using difference_type	= typename LIST::difference_type;

public:
	DecoratorList(LIST &list) : list_(list){}

public:
	// Immutable Methods

	const Pair *operator[](StringRef const &key) const{
		assert(!key.empty());
		return list_[key];
	}

	size_type size() const{
		return list_.size();
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

	bool erase(StringRef const &key){
		assert(!key.empty());
		return list_.erase(key);
	}

	bool insert(OPair &&data){
		return list_.insert( std::move(data) );
	}

private:
	LIST	&list_;
};


} // namespace


#endif
