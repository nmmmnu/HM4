#include "vectorlist.h"

#include "binarysearch.h"

#include <algorithm>
#include <cassert>

namespace hm4{

namespace{
	int comp(const Pair *p, std::string_view const key){
		return p->cmp(key);
	}

	template<class T>
	auto binarySearch(T &v, std::string_view const key){
		return ::binarySearch(std::begin(v), std::end(v), key, comp);
	}
} // anonymous namespace

auto VectorList::find(std::string_view const key, std::true_type) const noexcept -> iterator{
	// better Pair::check(key), but might fail because of the caller.
	assert(!key.empty());

	const auto &[found, it] = binarySearch(vector_, key);

	return found ? it : end();
}

auto VectorList::find(std::string_view const key, std::false_type) const noexcept -> iterator{
	assert(!key.empty());

	const auto &[found, it] = binarySearch(vector_, key);

	return it;
}

bool VectorList::insert(
		std::string_view const key, std::string_view const val,
		uint32_t const expires, uint32_t const created){

	auto newdata = Pair::up::create(*allocator_, key, val, expires, created);

	if (!newdata)
		return false;

	const auto &[found, it] = binarySearch(vector_, key);

	if (found){
		// key exists, overwrite, do not shift

		Pair *olddata = *it;

		// check if the data in database is valid
		if (! newdata->isValid(*olddata) ){
			// newdata will be magically destroyed.
			return false;
		}

		lc_.upd(olddata->bytes(), newdata->bytes());

		// assign new pair
		*it = newdata.release();

		// deallocate old pair
		allocator_->deallocate(olddata);

		return true;
	}

	// key not exists, shift, then add

	try{
		vector_.insert(it, newdata.get());
		lc_.inc(newdata->bytes());
	}catch(...){
		return false;
	}

	newdata.release();

	return true;
}

bool VectorList::erase(std::string_view const key){
	// better Pair::check(key), but might fail because of the caller.
	assert(!key.empty());

	const auto &[found, it] = binarySearch(vector_, key);

	if (! found){
		// the key does not exists in the vector.
		return true;
	}

	lc_.dec((*it)->bytes());

	allocator_->deallocate(*it);

	vector_.erase(it);

	return true;
}

bool VectorList::clear(){
	if (allocator_->reset() == false){
		std::for_each(std::begin(vector_), std::end(vector_), [this](void *p){
			allocator_->deallocate(p);
		});
	}

	vector_.clear();
	lc_.clr();
	return true;
}

} // namespace

