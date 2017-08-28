#ifndef _BLACK_HOLE_LIST_LIST_H
#define _BLACK_HOLE_LIST_LIST_H

#include "ilist.h"


namespace hm4{


class BlackHoleList : public IList<BlackHoleList, true>{
public:
	using Iterator = const Pair *;

public:
	constexpr
	bool clear() const noexcept{
		return true;
	}

	constexpr
	const Pair &operator[](const StringRef &) const noexcept{
		return Pair::zero();
	}

	constexpr
	bool erase(const StringRef &) const noexcept{
		return true;
	}

	constexpr
	size_type size(bool const = false) const noexcept{
		return 0;
	}

	constexpr
	size_t bytes() const noexcept{
		return 0;
	}

public:
	constexpr
	Iterator lowerBound(const StringRef &) const noexcept{
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

private:
	friend class IList<BlackHoleList, true>;

	constexpr
	bool insertT_(const Pair &) const {
		return true;
	}
};


} // namespace


#endif
