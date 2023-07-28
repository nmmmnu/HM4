#ifndef _VECTORLIST_H
#define _VECTORLIST_H

#include "ilist.h"
#include "listcounter.h"

#include "pointer_iterator.h"

#include "mynarrow.h"

#include <vector>

namespace hm4{

template<class T_Allocator>
class VectorList{
	using OVector	= std::vector<Pair *>;

public:
	using Allocator		= T_Allocator;
	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

public:
	using iterator = pointer_iterator<OVector::const_iterator>;

public:
	VectorList(Allocator &allocator) : allocator_(& allocator){}

	VectorList(VectorList &&other) = default;
	~VectorList(){
		clear();
	}

private:
	OVector		vector_;
	ListCounter	lc_;
	Allocator	*allocator_;

public:
	bool clear();

	bool erase_(std::string_view key);

	Pair const &operator[](size_type const index) const{
		return *vector_[index];
	}

	template<class PFactory>
	iterator insertF(PFactory &factory);

	auto size() const{
		return lc_.size();
	}

	auto mutable_size() const{
		return size();
	}

	void mutable_notify(const Pair *, PairFactoryMutableNotifyMessage const &msg){
		lc_.upd(msg.bytes_old, msg.bytes_new);
	}

	auto bytes() const{
		return lc_.bytes();
	}

	auto mutable_bytes() const{
		return bytes();
	}

	constexpr static void crontab(){
	}

	const Allocator &getAllocator() const{
		return *allocator_;
	}

	Allocator &getAllocator(){
		return *allocator_;
	}

public:
	template<bool B>
	iterator find(std::string_view const key, std::bool_constant<B> exact) const;

	iterator begin() const noexcept{
		return std::begin(vector_);
	}

	iterator end() const noexcept{
		return std::end(vector_);
	}
};

} // namespace



#endif
