#include "vectorlist.h"

#include "binarysearch.h"

#include <algorithm>
#include <cassert>

#include "pmallocator.h"
#include "stdallocator.h"
#include "arenaallocator.h"
#include "simulatedarenaallocator.h"

namespace hm4{

namespace{
	template<class T>
	auto binarySearch(T &v, std::string_view const key){

		auto comp = [](const Pair *p, std::string_view const key){
			return p->cmp(key);
		};

		// there are not much improvement,
		// but just to be on the safe side

		auto prefetch = [](const Pair *p){
			constexpr bool use_prefetch = true;

			if constexpr(use_prefetch)
				builtin_prefetch(p, 0, 1);
		};

		return ::binarySearchPrefetch(std::begin(v), std::end(v), key, comp, prefetch);
	}
} // anonymous namespace

template<class T_Allocator>
auto VectorList<T_Allocator>::find(std::string_view const key, std::true_type) const noexcept -> iterator{
	// better Pair::check(key), but might fail because of the caller.
	assert(!key.empty());

	const auto &[found, it] = binarySearch(vector_, key);

	return found ? it : end();
}

template<class T_Allocator>
auto VectorList<T_Allocator>::find(std::string_view const key, std::false_type) const noexcept -> iterator{
	assert(!key.empty());

	const auto &[found, it] = binarySearch(vector_, key);

	return it;
}

template<class T_Allocator>
template<class PFactory>
auto VectorList<T_Allocator>::insertLazyPair_(PFactory &&factory) -> iterator{
	auto const &key = factory.getKey();

	const auto &[found, it] = binarySearch(vector_, key);

	if (found){
		// key exists, overwrite, do not shift

		Pair *olddata = *it;

		// try update pair in place.

		if (factory(olddata, *this)){
			// successfully updated.

			return { it };
		}

		auto newdata = factory(getAllocator());

		if (!newdata)
			return this->end();

		if constexpr(config::LIST_CHECK_PAIR_FOR_REPLACE){
			// check if the data in database is valid
			if (! newdata->isValidForReplace(*olddata) ){
				// newdata will be magically destroyed.
				return this->end();
			}
		}

		lc_.upd(olddata->bytes(), newdata->bytes());

		// assign new pair
		*it = newdata.release();

		// deallocate old pair
		using namespace MyAllocator;

		deallocate(allocator_, olddata);

		return { it };
	}

	auto newdata = factory(getAllocator());

	if (!newdata)
		return this->end();

	// key not exists, shift, then add

	try{
		auto it2 = vector_.insert(it, newdata.get());

		lc_.inc(newdata->bytes());

		newdata.release();

		return { it2 };
	}catch(...){
		// newdata will be deallocated...
		return this->end();
	}
}

template<class T_Allocator>
bool VectorList<T_Allocator>::erase_(std::string_view const key){
	// better Pair::check(key), but might fail because of the caller.
	assert(!key.empty());

	const auto &[found, it] = binarySearch(vector_, key);

	if (! found){
		// the key does not exists in the vector.
		return false;
	}

	lc_.dec((*it)->bytes());

	using namespace MyAllocator;
	deallocate(allocator_, *it);

	vector_.erase(it);

	return true;
}

template<class T_Allocator>
bool VectorList<T_Allocator>::clear(){
	if (allocator_->reset() == false){
		std::for_each(std::begin(vector_), std::end(vector_), [this](void *p){
			using namespace MyAllocator;
			deallocate(allocator_, p);
		});
	}

	vector_.clear();
	lc_.clr();
	return true;
}

// ==============================

template class VectorList<MyAllocator::PMAllocator>;
template class VectorList<MyAllocator::STDAllocator>;
template class VectorList<MyAllocator::ArenaAllocator>;
template class VectorList<MyAllocator::SimulatedArenaAllocator>;

template auto VectorList<MyAllocator::PMAllocator>		::insertLazyPair_(PairFactory::Normal		&&factory) -> iterator;
template auto VectorList<MyAllocator::STDAllocator>		::insertLazyPair_(PairFactory::Normal		&&factory) -> iterator;
template auto VectorList<MyAllocator::ArenaAllocator>		::insertLazyPair_(PairFactory::Normal		&&factory) -> iterator;
template auto VectorList<MyAllocator::SimulatedArenaAllocator>	::insertLazyPair_(PairFactory::Normal		&&factory) -> iterator;

template auto VectorList<MyAllocator::PMAllocator>		::insertLazyPair_(PairFactory::Tombstone	&&factory) -> iterator;
template auto VectorList<MyAllocator::STDAllocator>		::insertLazyPair_(PairFactory::Tombstone	&&factory) -> iterator;
template auto VectorList<MyAllocator::ArenaAllocator>		::insertLazyPair_(PairFactory::Tombstone	&&factory) -> iterator;
template auto VectorList<MyAllocator::SimulatedArenaAllocator>	::insertLazyPair_(PairFactory::Tombstone	&&factory) -> iterator;

template auto VectorList<MyAllocator::PMAllocator>		::insertLazyPair_(PairFactory::Clone		&&factory) -> iterator;
template auto VectorList<MyAllocator::STDAllocator>		::insertLazyPair_(PairFactory::Clone		&&factory) -> iterator;
template auto VectorList<MyAllocator::ArenaAllocator>		::insertLazyPair_(PairFactory::Clone		&&factory) -> iterator;
template auto VectorList<MyAllocator::SimulatedArenaAllocator>	::insertLazyPair_(PairFactory::Clone		&&factory) -> iterator;

} // namespace

