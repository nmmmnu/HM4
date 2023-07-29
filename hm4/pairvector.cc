#include "pairvector.h"

#include "binarysearch.h"

#include "ilist_updateinplace.h"

#include <cassert>
#include <cstring>		// memmove
#include <stdexcept>		// std::bad_alloc

#include "pmallocator.h"
#include "stdallocator.h"
#include "arenaallocator.h"
#include "simulatedarenaallocator.h"

namespace hm4{

namespace {
	inline void xmemmove(void *dest, const void *src, size_t size){
		memmove(dest, src, size);
	}

	template<typename T>
	void shiftL_(T *pos, T *end){
		xmemmove(
			pos,
			pos + 1,
			(end - pos - 1) * sizeof(T)
		);
	}

	template<typename T>
	void shiftR_(T *pos, T *end){
		xmemmove(
			pos + 1,
			pos,
			(end - pos + 1) * sizeof(T)
		);
	}

	// ---------------------

	template<class It>
	auto binarySearch(It begin, It end, std::string_view const key){

		auto comp = [](const Pair *p, std::string_view const key){
			return p->cmp(key);
		};

		return ::binarySearch(begin, end, key, comp);
	}
} // anonymous namespace

template<class T_Allocator>
bool PairVector<T_Allocator>::destruct(Allocator &allocator) noexcept{
	if (allocator.reset() == false){
		for(auto it = begin_(); it != end_(); ++it){
			using namespace MyAllocator;
			deallocate(allocator, *it);
		};
	}

	return true;
}

template<class T_Allocator>
void PairVector<T_Allocator>::assign_(Pair **first, Pair **last){
	auto const len = static_cast<size_type>(last - first);

	if (size() + len > capacity())
		throw std::bad_alloc{};

	std::move(first, last, end_());

	size_ += len;
}

template<class T_Allocator>
void PairVector<T_Allocator>::split(PairVector &other){
	assert(other.size() == 0);

	auto const len = size_ / 2;

	other.assign_(begin_() + len, end_());

	size_ -= other.size();
}

template<class T_Allocator>
void PairVector<T_Allocator>::merge(PairVector &other){
	assign_(other.begin_(), other.end_());

	other.size_ = 0;
}

template<class T_Allocator>
template<bool ExactMatch>
auto PairVector<T_Allocator>::find(std::string_view const key, std::bool_constant<ExactMatch>) const noexcept -> iterator{
	assert(!key.empty());

	auto const &[found, it] = binarySearch(begin_(), end_(), key);

	if constexpr(ExactMatch)
		return found ? it : end();
	else
		return it;
}

template<class T_Allocator>
template<class PFactory>
auto PairVector<T_Allocator>::insertF(PFactory &factory, Allocator &allocator, ListCounter &lc) -> iterator{
	auto const &key = factory.getKey();

	auto [found, it] = binarySearch(begin_(), end_(), key);

	if (found){
		// key exists, overwrite, do not shift

		Pair *olddata = *it;

		if constexpr(config::LIST_CHECK_PAIR_FOR_REPLACE)
			if (!isValidForReplace(factory.getCreated(), *olddata))
				return end();

		// try update pair in place.
		if (tryUpdateInPlaceLC(allocator, olddata, factory, lc)){
			// successfully updated.
			return it;
		}

		auto newdata = Pair::smart_ptr::create(allocator, factory);

		if (!newdata)
			return this->end();

		lc.upd(olddata->bytes(), newdata->bytes());

		// assign new pair
		*it = newdata.release();

		// deallocate old pair
		using namespace MyAllocator;
		deallocate(allocator, olddata);

		return it;
	}

	if (size() == capacity())
		throw std::bad_alloc{};

	auto newdata = Pair::smart_ptr::create(allocator, factory);

	if (!newdata)
		return end();

	// make space, exception free, so no need to protect with unique_ptr.
	shiftR_(it, end_());

	lc.inc(newdata->bytes());

	*it = newdata.release();

	++size_;

	return it;
}

template<class T_Allocator>
bool PairVector<T_Allocator>::erase_(std::string_view const &key, Allocator &allocator, ListCounter *lc){
	assert(!key.empty());

	auto [found, it] = binarySearch(begin_(), end_(), key);

	if (!found)
		return false;

	if (lc)
		lc->dec((*it)->bytes());

	using namespace MyAllocator;
	deallocate(allocator, *it);

	shiftL_(it, end_());

	--size_;

	return true;
}

// ==============================

template class PairVector<MyAllocator::PMAllocator>;
template class PairVector<MyAllocator::STDAllocator>;
template class PairVector<MyAllocator::ArenaAllocator>;
template class PairVector<MyAllocator::SimulatedArenaAllocator>;

template auto PairVector<MyAllocator::PMAllocator>		::find(std::string_view const key, std::true_type ) const -> iterator;
template auto PairVector<MyAllocator::STDAllocator>		::find(std::string_view const key, std::true_type ) const -> iterator;
template auto PairVector<MyAllocator::ArenaAllocator>		::find(std::string_view const key, std::true_type ) const -> iterator;
template auto PairVector<MyAllocator::SimulatedArenaAllocator>	::find(std::string_view const key, std::true_type ) const -> iterator;

template auto PairVector<MyAllocator::PMAllocator>		::find(std::string_view const key, std::false_type) const -> iterator;
template auto PairVector<MyAllocator::STDAllocator>		::find(std::string_view const key, std::false_type) const -> iterator;
template auto PairVector<MyAllocator::ArenaAllocator>		::find(std::string_view const key, std::false_type) const -> iterator;
template auto PairVector<MyAllocator::SimulatedArenaAllocator>	::find(std::string_view const key, std::false_type) const -> iterator;

template auto PairVector<MyAllocator::PMAllocator>		::insertF(PairFactory::Normal		&factory, Allocator &allocator, ListCounter &lc) -> iterator;
template auto PairVector<MyAllocator::STDAllocator>		::insertF(PairFactory::Normal		&factory, Allocator &allocator, ListCounter &lc) -> iterator;
template auto PairVector<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Normal		&factory, Allocator &allocator, ListCounter &lc) -> iterator;
template auto PairVector<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Normal		&factory, Allocator &allocator, ListCounter &lc) -> iterator;

template auto PairVector<MyAllocator::PMAllocator>		::insertF(PairFactory::Expires		&factory, Allocator &allocator, ListCounter &lc) -> iterator;
template auto PairVector<MyAllocator::STDAllocator>		::insertF(PairFactory::Expires		&factory, Allocator &allocator, ListCounter &lc) -> iterator;
template auto PairVector<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Expires		&factory, Allocator &allocator, ListCounter &lc) -> iterator;
template auto PairVector<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Expires		&factory, Allocator &allocator, ListCounter &lc) -> iterator;

template auto PairVector<MyAllocator::PMAllocator>		::insertF(PairFactory::Tombstone	&factory, Allocator &allocator, ListCounter &lc) -> iterator;
template auto PairVector<MyAllocator::STDAllocator>		::insertF(PairFactory::Tombstone	&factory, Allocator &allocator, ListCounter &lc) -> iterator;
template auto PairVector<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Tombstone	&factory, Allocator &allocator, ListCounter &lc) -> iterator;
template auto PairVector<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Tombstone	&factory, Allocator &allocator, ListCounter &lc) -> iterator;

template auto PairVector<MyAllocator::PMAllocator>		::insertF(PairFactory::Clone		&factory, Allocator &allocator, ListCounter &lc) -> iterator;
template auto PairVector<MyAllocator::STDAllocator>		::insertF(PairFactory::Clone		&factory, Allocator &allocator, ListCounter &lc) -> iterator;
template auto PairVector<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Clone		&factory, Allocator &allocator, ListCounter &lc) -> iterator;
template auto PairVector<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Clone		&factory, Allocator &allocator, ListCounter &lc) -> iterator;

template auto PairVector<MyAllocator::PMAllocator>		::insertF(PairFactory::IFactory		&factory, Allocator &allocator, ListCounter &lc) -> iterator;
template auto PairVector<MyAllocator::STDAllocator>		::insertF(PairFactory::IFactory		&factory, Allocator &allocator, ListCounter &lc) -> iterator;
template auto PairVector<MyAllocator::ArenaAllocator>		::insertF(PairFactory::IFactory		&factory, Allocator &allocator, ListCounter &lc) -> iterator;
template auto PairVector<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::IFactory		&factory, Allocator &allocator, ListCounter &lc) -> iterator;

} // namespace hm4


