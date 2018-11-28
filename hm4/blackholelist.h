#ifndef _BLACK_HOLE_LIST_LIST_H
#define _BLACK_HOLE_LIST_LIST_H

#include "ilist.h"


namespace hm4{


class BlackHoleList{
public:
	using size_type	= config::size_type;

	using iterator	= const Pair *;

public:
	constexpr static
	bool clear(){
		return true;
	}

	constexpr
	const Pair *operator[](StringRef const &) const{
		return nullptr;
	}

	constexpr static
	bool erase(StringRef const &){
		return true;
	}

	constexpr static
	bool insert(OPair const &){
		return true;
	}

	constexpr static
	size_type size(){
		return 0;
	}

	constexpr static
	size_t bytes(){
		return 0;
	}

public:
	constexpr static
	iterator lowerBound(StringRef const &){
		return nullptr;
	}

	constexpr static
	iterator begin(){
		return nullptr;
	}

	constexpr static
	iterator end(){
		return nullptr;
	}
};


} // namespace


#endif
