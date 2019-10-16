#include "vectorlist.h"

#include "binarysearch.h"

namespace hm4{

namespace{
	int comp(OPair const &p, std::string_view const key){
		return p.cmp(key);
	}

	template<class T>
	auto binarySearch(T &v, std::string_view const key){
		return ::binarySearch(std::begin(v), std::end(v), key, comp);
	}
} // anonymous namespace

auto VectorList::find(std::string_view const key, std::true_type) const noexcept -> iterator{
	assert(!key.empty());

	auto const x = binarySearch(vector_, key);

	return x.found ? x.it : end();
}

auto VectorList::find(std::string_view const key, std::false_type) const noexcept -> iterator{
	assert(!key.empty());

	auto const x = binarySearch(vector_, key);

	return x.it;
}

bool VectorList::insert(OPair&& newdata){
	assert(newdata);

	std::string_view const key = newdata->getKey();

	auto const x = binarySearch(vector_, key);

	if (x.found){
		// key exists, overwrite, do not shift

		OPair &olddata = *x.it;

		// check if the data in database is valid
		if (! newdata->isValid(*olddata) ){
			// newdata will be magically destroyed.
			return false;
		}

		lc_.upd(olddata->bytes(), newdata->bytes());

		// copy assignment
		olddata = std::move(newdata);

		return true;
	}

	// key not exists, shift, then add

	try{
		auto const newit = vector_.insert(x.it, std::move(newdata));
		lc_.inc(newit->get()->bytes());
	}catch(...){
	}

	return true;
}

bool VectorList::erase(std::string_view const key){
	assert(!key.empty());

	auto const x = binarySearch(vector_, key);

	if (! x.found){
		// the key does not exists in the vector.
		return true;
	}

	lc_.dec(x.it->get()->bytes());

	vector_.erase(x.it);

	return true;
}

} // namespace

