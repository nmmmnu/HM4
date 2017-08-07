#ifndef _MY_LIST_H
#define _MY_LIST_H

#include <cstdint>

#include <cassert>

#include "pair.h"


namespace hm4{


class IListConf{
public:
	using size_type		= uint64_t;
	using difference_type	= int64_t;
};

// ==============================

template <class T, bool MUTABLE>
class IList : public IListConf{
protected:
	constexpr static size_type PRINT_COUNT	= 10;

public:
	constexpr static bool MUTABLE_TAG	= MUTABLE;

public:
	// Immutable Methods

	void print(size_type count = PRINT_COUNT) const{
		for(const Pair &p : *self() ){
			p.print();
			if (--count == 0)
				return;
		}
	}

	bool empty() const{
		return ! self()->size(true);
	}

public:
	// Mutable Methods

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

		if (pair)
			return self()->insertT_(std::move(pair));
		else
			return false;
	}

private:
	const T *self() const{
		return static_cast<const T*>(this);
	}

	T *self(){
		return static_cast<T*>(this);
	}
};

} // namespace

#endif

