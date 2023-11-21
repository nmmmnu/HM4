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
	constexpr static const char *getName(){
		return "BlackHoleList";
	}

public:
	constexpr static
	bool clear(){
		return true;
	}

	constexpr static
	bool erase_(std::string_view ){
		return true;
	}

	template<class PFactory>
	InsertResult insertF(PFactory &){
		return InsertResult::skipInserted();
	}

	auto const &mutable_list() const{
		return *this;
	}

	constexpr static void mutable_notify(const Pair *, PairFactoryMutableNotifyMessage const &){
	}

	constexpr static
	size_type size(){
		return 0;
	}

	constexpr static
	bool empty(){
		return true;
	}

	constexpr static
	size_t bytes(){
		return 0;
	}

	constexpr static void crontab(){
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

