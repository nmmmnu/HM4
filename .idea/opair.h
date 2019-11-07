#ifndef PAIR_OWNER_H
#define PAIR_OWNER_H

#include "pair.h"
#include "pmdeleter.h"

namespace hm4{



class OPair{
public:
	template<class Allocator>
	OPair(Allocator &a,
		std::string_view const key, std::string_view const val,
		uint32_t const expires = 0, uint32_t const created = 0) : deleter(a.getDeleter()){

		if (	key.size() == 0				||
			key.size() > PairConf::MAX_KEY_SIZE	||
			val.size() > PairConf::MAX_VAL_SIZE	)
			return;

		pp = a. template allocate<Pair>(Pair::bytes(key.size(), val.size()));

		if (!pp)
			return;

		Pair::copy_(pp, key, val, expires, created);
	}

	template<class Allocator>
	OPair(Allocator &a, const Pair *src) : deleter(a.getDeleter()){
		if (!src)
			return;

		pp = a. template allocate<Pair>(src->bytes());

		if (!pp)
			return;

		memcpy(pp, src, src->bytes());
	}

	template<class Allocator>
	OPair(Allocator &a, const Pair &src) : OPair(a, & src){}

	~OPair(){
		// virtual call
		(*deleter)(pp);
	}

	OPair(OPair &&other) : pp(other.pp), deleter(other.deleter){
		other.pp = nullptr;
	}

	OPair &operator =(OPair &&other){
		using std::swap;

		swap(pp,	other.pp	);
		swap(deleter,	other.deleter	);

		return *this;
	}

public:
	operator bool() const noexcept{
		return pp;
	}

	const Pair &operator *() const noexcept{
		return *pp;
	}

	const Pair *operator ->() const noexcept{
		return pp;
	}

	const Pair *get() const noexcept{
		return pp;
	}

	int cmp(std::string_view const key) const noexcept{
		return pp->cmp(key);
	}

protected:
	Pair *pp	= nullptr;
	MyAllocator::PMDeleter *deleter;
};


// ==============================

inline void print(OPair const &pair){
	if (pair)
		pair->print();
	else
		printf("%s\n", PairConf::EMPTY_MESSAGE);
}

} // namespace

#endif

