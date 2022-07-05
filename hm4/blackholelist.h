#ifndef _BLACK_HOLE_LIST_LIST_H
#define _BLACK_HOLE_LIST_LIST_H

#include "ilist.h"
#include "nullallocator.h"

namespace hm4{



class BlackHoleList{
public:
	using Allocator		= MyAllocator::NULLAllocator;

	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

	using iterator		= const Pair *;

public:
	constexpr const Allocator &getAllocator() const{
		return MyAllocator::var_NULLAllocator;
	}

public:
	constexpr static
	bool clear(){
		return true;
	}

	constexpr static
	bool erase(std::string_view ){
		return true;
	}

	constexpr static
	iterator insert(
			std::string_view, std::string_view,
			uint32_t = 0, uint32_t = 0
		){
		return nullptr;
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
	template<bool B>
	constexpr static
	iterator find(std::string_view, std::bool_constant<B>){
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
