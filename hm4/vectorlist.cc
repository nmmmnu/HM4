#include "vectorlist.h"

namespace hm4{

auto VectorList::binarySearch_(const StringRef &key) const -> BinarySearchResult<size_type>{
	assert(!key.empty());

	auto comp = [](const auto &list, auto const index, const auto &key){
		return list.cmpAt(index, key);
	};

	return binarySearch(*this, size_type{0}, size(), key, comp);
}

bool VectorList::insert(OPair&& newdata){
	assert(newdata);

	const StringRef &key = newdata->getKey();

	const auto x = binarySearch_(key);

	if (x.found){
		// key exists, overwrite, do not shift

		OPair &olddata = vector_[ x.pos ];

		// check if the data in database is valid
		if (! newdata->isValid(*olddata) ){
			// newdata will be magically destroyed.
			return false;
		}

		dataSize_ = dataSize_
					- olddata->bytes()
					+ newdata->bytes();

		// copy assignment
		olddata = std::move(newdata);

		return true;
	}

	// key not exists, shift, then add

	try{
		const auto it = beginOffset__(vector_, x.pos);
		const auto newit = vector_.insert( it, std::move(newdata));
		dataSize_ += newit->get()->bytes();
	}catch(...){
	}

	return true;
}

bool VectorList::erase(const StringRef &key){
	assert(!key.empty());

	const auto x = binarySearch_(key);

	if (! x.found){
		// the key does not exists in the vector.
		return true;
	}

	const auto it = beginOffset__(vector_, x.pos);

	dataSize_ -= it->get()->bytes();

	vector_.erase(it);

	return true;
}

} // namespace

