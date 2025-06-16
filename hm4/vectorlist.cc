#include "vectorlist.h"

#include "binarysearch.h"

#include "ilist/updateinplace.h"

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
				builtin_prefetch(p);
		};

		return ::binarySearchPrefetch(std::begin(v), std::end(v), key, comp, prefetch);
	}
} // anonymous namespace

template<class T_Allocator>
auto VectorList<T_Allocator>::find(std::string_view const key) const -> iterator{
	assert(!key.empty());

	const auto &[found, it] = binarySearch(vector_, key);

	return iterator{ it };
}

template<class T_Allocator>
const Pair *VectorList<T_Allocator>::getPair(std::string_view const key) const{
	assert(!key.empty());

	const auto &[found, it] = binarySearch(vector_, key);

	return found ? *it : nullptr;
}

template<class T_Allocator>
template<class PFactory>
auto VectorList<T_Allocator>::insertF(PFactory &factory) -> InsertResult{
	if (!factory.valid())
		return InsertResult::errorInvalid();

	auto const &key = factory.getKey();

	const auto &[found, it] = binarySearch(vector_, key);

	if (found){
		// key exists, overwrite, do not shift

		Pair *olddata = *it;

		if constexpr(config::LIST_CHECK_PAIR_FOR_REPLACE)
			if (!isValidForReplace(factory.getCreated(), *olddata))
				return InsertResult::skipInserted();

		// try update pair in place.
		if (tryUpdateInPlaceLC(getAllocator(), olddata, factory, lc_)){
			// successfully updated.
			return InsertResult::updatedInPlace(olddata);
		}

		auto newdata = Pair::smart_ptr::create(getAllocator(), factory);

		if (!newdata)
			return InsertResult::errorNoMemory();

		lc_.upd(olddata->bytes(), newdata->bytes());

		// assign new pair
		*it = newdata.release();

		// deallocate old pair
		using namespace MyAllocator;
		deallocate(allocator_, olddata);

		return InsertResult::replaced(*it);
	}

	auto newdata = Pair::smart_ptr::create(getAllocator(), factory);

	if (!newdata)
		return InsertResult::errorNoMemory();

	// key not exists, shift, then add

	try{
		auto it2 = vector_.insert(it, newdata.get());

		lc_.inc(newdata->bytes());

		newdata.release();

		return InsertResult::inserted(*it2);
	}catch(...){
		// newdata will be deallocated...
		return InsertResult::error();
	}
}

template<class T_Allocator>
InsertResult VectorList<T_Allocator>::erase_(std::string_view const key){
	// better Pair::check(key), but might fail because of the caller.
	assert(!key.empty());

	const auto &[found, it] = binarySearch(vector_, key);

	if (! found){
		// the key does not exists in the vector.
		return InsertResult::skipDeleted();
	}

	lc_.dec((*it)->bytes());

	using namespace MyAllocator;
	deallocate(allocator_, *it);

	vector_.erase(it);

	return InsertResult::deleted();
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

template auto VectorList<MyAllocator::PMAllocator>		::insertF(PairFactory::Normal		&factory) -> InsertResult;
template auto VectorList<MyAllocator::STDAllocator>		::insertF(PairFactory::Normal		&factory) -> InsertResult;
template auto VectorList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Normal		&factory) -> InsertResult;
template auto VectorList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Normal		&factory) -> InsertResult;

template auto VectorList<MyAllocator::PMAllocator>		::insertF(PairFactory::Expires		&factory) -> InsertResult;
template auto VectorList<MyAllocator::STDAllocator>		::insertF(PairFactory::Expires		&factory) -> InsertResult;
template auto VectorList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Expires		&factory) -> InsertResult;
template auto VectorList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Expires		&factory) -> InsertResult;

template auto VectorList<MyAllocator::PMAllocator>		::insertF(PairFactory::Tombstone	&factory) -> InsertResult;
template auto VectorList<MyAllocator::STDAllocator>		::insertF(PairFactory::Tombstone	&factory) -> InsertResult;
template auto VectorList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Tombstone	&factory) -> InsertResult;
template auto VectorList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Tombstone	&factory) -> InsertResult;

template auto VectorList<MyAllocator::PMAllocator>		::insertF(PairFactory::Clone		&factory) -> InsertResult;
template auto VectorList<MyAllocator::STDAllocator>		::insertF(PairFactory::Clone		&factory) -> InsertResult;
template auto VectorList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Clone		&factory) -> InsertResult;
template auto VectorList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Clone		&factory) -> InsertResult;

template auto VectorList<MyAllocator::PMAllocator>		::insertF(PairFactory::IFactory		&factory) -> InsertResult;
template auto VectorList<MyAllocator::STDAllocator>		::insertF(PairFactory::IFactory		&factory) -> InsertResult;
template auto VectorList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::IFactory		&factory) -> InsertResult;
template auto VectorList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::IFactory		&factory) -> InsertResult;

} // namespace

