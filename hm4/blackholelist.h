#ifndef _BLACK_HOLE_LIST_LIST_H
#define _BLACK_HOLE_LIST_LIST_H

#include "ilist.h"


namespace hm4{


class BlackHoleList{
public:
	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

	using Iterator = const Pair *;

public:
	constexpr
	bool clear() const noexcept{
		return true;
	}

	constexpr
	const Pair *operator[](StringRef const &) const noexcept{
		return nullptr;
	}

	constexpr
	bool erase(StringRef const &) const noexcept{
		return true;
	}

	constexpr
	bool insert(OPair const &) const {
		return true;
	}

	constexpr
	size_type size() const noexcept{
		return 0;
	}

	constexpr
	size_t bytes() const noexcept{
		return 0;
	}

public:
	constexpr
	Iterator lowerBound(StringRef const &) const noexcept{
		return nullptr;
	}

	constexpr
	Iterator begin() const{
		return nullptr;
	}

	constexpr
	Iterator end() const{
		return nullptr;
	}
};


} // namespace


#endif
