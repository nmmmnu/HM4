#ifndef _MY_LIST_H
#define _MY_LIST_H

#include <cstdint>

#include <cassert>

#include "pair.h"


namespace hm4{


class IList{
public:
	using size_type		= uint64_t;
	using difference_type	= int64_t;
};

// ==============================

template <class T>
class List : public IList{
protected:
	constexpr
	static size_type PRINT_COUNT = 10;

public:
	void print(size_type count = PRINT_COUNT) const{
		for(const Pair &p : *self() ){
			p.print();
			if (--count == 0)
				return;
		}
	}

	bool empty() const{
		return ! size(true);
	}

public:
	size_type size(bool const estimated = false) const{
		return static_cast<size_type>( self()->size(estimated) );
	}

private:
	const T *self() const{
		return static_cast<const T*>(this);
	}
};

// ==============================

template <class T>
class IMutableList : public List<T>{
public:
	bool insert(const Pair &pair){
		assert( pair );

		return self()->insertT_(pair);
	}

	bool insert(Pair &&pair){
		assert( pair );

		if (pair.isObserver()){
			// Observer must be copied
			return self()->insertT_( const_cast<const Pair &>(pair) );
		}else{
			// normal Pair can be forwarded
			return self()->insertT_(std::move(pair));
		}
	}

	template <class ...ARGS>
	bool emplace(ARGS ...args){
		Pair pair{ std::forward<ARGS>(args)... };

		return self()->insertT_(std::move(pair));
	}

private:
	T *self(){
		return static_cast<T*>(this);
	}
};


} // namespace

#endif

