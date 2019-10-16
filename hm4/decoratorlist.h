#ifndef DECORATOR_LIST_H_
#define DECORATOR_LIST_H_


#include "ilist.h"


namespace hm4{


template <class LIST>
class DecoratorList{
public:
	using iterator		= typename LIST::iterator;

	using size_type		= typename LIST::size_type;
	using difference_type	= typename LIST::difference_type;

public:
	DecoratorList(LIST &list) : list_(list){}

public:
	// Immutable Methods

	size_type size() const{
		return list_.size();
	}

	size_t bytes() const{
		return list_.bytes();
	}

public:
	iterator begin() const{
		return list_.begin();
	}

	iterator end() const{
		return list_.end();
	}

	template<bool B>
	iterator find(std::string_view const key, std::bool_constant<B> const exact) const{
		return list_.find(key, exact);
	}

public:
	// Mutable Methods

	bool clear(){
		return list_.clear();
	}

	bool erase(std::string_view const key){
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
