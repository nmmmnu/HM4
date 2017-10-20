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

template <class T, bool MU>
class IList : public IListConf{
protected:
	constexpr static size_type PRINT_COUNT	= 10;

public:
	constexpr static bool MUTABLE	= MU;

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

protected:
	size_type sizeViaIterator_() const{
		// Slooooow....
		size_type count = 0;

		auto bit = self()->begin();
		auto eit = self()->end();

		for(auto it = bit; it != eit; ++it)
			++count;

		return count;
	}

private:
	const T *self() const{
		return static_cast<const T*>(this);
	}
};

} // namespace

#endif

