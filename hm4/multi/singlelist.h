#ifndef DECORATOR_List_H_
#define DECORATOR_List_H_


#include "ilist.h"

#include <cassert>

namespace hm4::multi{



template <class List>
class SingleListBase{
public:
	using iterator		= typename List::iterator;

	using size_type		= typename List::size_type;
	using difference_type	= typename List::difference_type;

public:
	SingleListBase(List &list) : list_(& list){}

public:
	// Immutable Methods

	size_type size() const{
		return list_->size();
	}

	size_t bytes() const{
		return list_->bytes();
	}

public:
	iterator begin() const{
		return list_->begin();
	}

	iterator end() const{
		return list_->end();
	}

	template<bool B>
	iterator find(std::string_view const key, std::bool_constant<B> const exact) const{
		return list_->find(key, exact);
	}

protected:
	List	*list_;
};



template<class List, class = std::void_t<> >
class SingleList : public SingleListBase<List>{
public:
	using SingleListBase<List>::SingleListBase;
};



template<class List>
class SingleList<List, std::void_t<typename List::Allocator> > : public SingleListBase<List>{
public:
	using SingleListBase<List>::SingleListBase;

	using Allocator = typename List::Allocator;

	const auto &getAllocator() const{
		return list_->getAllocator();
	}

	auto &getAllocator(){
		return list_->getAllocator();
	}

public:
	// Mutable Methods

	bool clear(){
		return list_->clear();
	}

	bool erase(std::string_view const key){
		// better Pair::check(key), but might fail because of the caller.
		assert(!key.empty());

		return list_->erase(key);
	}

	auto insert(	std::string_view const key, std::string_view const val,
			uint32_t const expires = 0, uint32_t const created = 0
			){

		return hm4::insert(*this, key, val, expires, created);
	}

	auto insert(Pair const &src){
		return hm4::insert(*this, src);
	}

	auto insert(typename Pair::smart_ptr::type<Allocator> &&newdata){
		return list_->insert(std::move(newdata));
	}

protected:
	using SingleListBase<List>::list_;
};



} // namespace



#endif
