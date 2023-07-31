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
			size_t(end - pos) * sizeof(T)
		);
	}

	template<typename T>
	void shiftR_(T *pos, T *end){
		xmemmove(
			pos + 1,
			pos,
			size_t(end - pos) * sizeof(T)
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

template<class Allocator, size_t Capacity>
bool PairVector<Allocator,Capacity>::destruct(Allocator &allocator) noexcept{
	if (allocator.reset() == false){
		for(auto it = ptr_begin(); it != ptr_end(); ++it){
			using namespace MyAllocator;
			deallocate(allocator, *it);
		};
	}

	return true;
}

template<class Allocator, size_t Capacity>
void PairVector<Allocator,Capacity>::assign_(Pair **first, Pair **last){
	auto const len = static_cast<size_type>(last - first);

	if (size() + len > capacity())
		throw std::bad_alloc{};

	std::move(first, last, ptr_end());

	size_ += len;
}

template<class Allocator, size_t Capacity>
void PairVector<Allocator,Capacity>::split(PairVector &other){
	assert(other.size() == 0);

	auto const len = size_ / 2;

	other.assign_(ptr_begin() + len, ptr_end());

	size_ -= other.size();
}

template<class Allocator, size_t Capacity>
void PairVector<Allocator,Capacity>::merge(PairVector &other){
	assign_(other.ptr_begin(), other.ptr_end());

	other.size_ = 0;
}

template<class Allocator, size_t Capacity>
auto PairVector<Allocator,Capacity>::locateC_(std::string_view const key) const noexcept -> ConstLocateResultPtr{
	assert(!key.empty());

	auto const &[found, it] = binarySearch(ptr_begin(), ptr_end(), key);

	return { found, it };
}

template<class Allocator, size_t Capacity>
auto PairVector<Allocator,Capacity>::locateM_(std::string_view const key) noexcept -> LocateResultPtr{
	assert(!key.empty());

	auto const &[found, it] = binarySearch(ptr_begin(), ptr_end(), key);

	return { found, it };
}

template<class Allocator, size_t Capacity>
template<bool ExactMatch>
auto PairVector<Allocator,Capacity>::find(std::string_view const key, std::bool_constant<ExactMatch>) const noexcept -> iterator{
	auto const &[found, it] = locateC_(key);

	if constexpr(ExactMatch)
		return found ? iterator{ it } : end();
	else
		return iterator{ it };
}

template<class Allocator, size_t Capacity>
template<class PFactory>
auto PairVector<Allocator,Capacity>::insertF(PFactory &factory, Allocator &allocator, ListCounter &lc) -> iterator{
	auto const &key = factory.getKey();

	auto [found, it] = locateM_(key);

	if (found){
		// key exists, overwrite, do not shift

		Pair *olddata = *it;

		if constexpr(config::LIST_CHECK_PAIR_FOR_REPLACE)
			if (!isValidForReplace(factory.getCreated(), *olddata))
				return end();

		// try update pair in place.
		if (tryUpdateInPlaceLC(allocator, olddata, factory, lc)){
			// successfully updated.
			return iterator{ it };
		}

		auto newdata = Pair::smart_ptr::create(allocator, factory);

		if (!newdata)
			return end();

		lc.upd(olddata->bytes(), newdata->bytes());

		// assign new pair
		*it = newdata.release();

		// deallocate old pair
		using namespace MyAllocator;
		deallocate(allocator, olddata);

		return iterator{ it };
	}

	if (size() == capacity())
		throw std::bad_alloc{};

	auto newdata = Pair::smart_ptr::create(allocator, factory);

	if (!newdata)
		return end();

	// make space, exception free, so no need to protect with unique_ptr.
	shiftR_(it, ptr_end());

	lc.inc(newdata->bytes());

	*it = newdata.release();

	++size_;

	return iterator{ it };
}

template<class Allocator, size_t Capacity>
bool PairVector<Allocator,Capacity>::erase_(std::string_view const &key, Allocator &allocator, ListCounter &lc){
	auto [found, it] = locateM_(key);

	if (!found)
		return false;

	lc.dec((*it)->bytes());

	using namespace MyAllocator;
	deallocate(allocator, *it);

	shiftL_(it, ptr_end());

	--size_;

	return true;
}

// ==============================

template class PairVector	<MyAllocator::PMAllocator	,    2>;
template auto PairVector	<MyAllocator::PMAllocator	,    2>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    2>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    2>		::insertF(PairFactory::Normal		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    2>		::insertF(PairFactory::Expires		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    2>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    2>		::insertF(PairFactory::Clone		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    2>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::PMAllocator	,    3>;
template auto PairVector	<MyAllocator::PMAllocator	,    3>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    3>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    3>		::insertF(PairFactory::Normal		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    3>		::insertF(PairFactory::Expires		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    3>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    3>		::insertF(PairFactory::Clone		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    3>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::PMAllocator	,    4>;
template auto PairVector	<MyAllocator::PMAllocator	,    4>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    4>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    4>		::insertF(PairFactory::Normal		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    4>		::insertF(PairFactory::Expires		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    4>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    4>		::insertF(PairFactory::Clone		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    4>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::PMAllocator	,    8>;
template auto PairVector	<MyAllocator::PMAllocator	,    8>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    8>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    8>		::insertF(PairFactory::Normal		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    8>		::insertF(PairFactory::Expires		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    8>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    8>		::insertF(PairFactory::Clone		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,    8>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::PMAllocator	,   16>;
template auto PairVector	<MyAllocator::PMAllocator	,   16>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,   16>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,   16>		::insertF(PairFactory::Normal		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,   16>		::insertF(PairFactory::Expires		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,   16>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,   16>		::insertF(PairFactory::Clone		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,   16>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::PMAllocator	,   32>;
template auto PairVector	<MyAllocator::PMAllocator	,   32>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,   32>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,   32>		::insertF(PairFactory::Normal		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,   32>		::insertF(PairFactory::Expires		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,   32>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,   32>		::insertF(PairFactory::Clone		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,   32>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::PMAllocator	,   64>;
template auto PairVector	<MyAllocator::PMAllocator	,   64>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,   64>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,   64>		::insertF(PairFactory::Normal		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,   64>		::insertF(PairFactory::Expires		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,   64>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,   64>		::insertF(PairFactory::Clone		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,   64>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::PMAllocator	,  128>;
template auto PairVector	<MyAllocator::PMAllocator	,  128>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,  128>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,  128>		::insertF(PairFactory::Normal		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,  128>		::insertF(PairFactory::Expires		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,  128>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,  128>		::insertF(PairFactory::Clone		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,  128>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::PMAllocator	,  256>;
template auto PairVector	<MyAllocator::PMAllocator	,  256>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,  256>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,  256>		::insertF(PairFactory::Normal		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,  256>		::insertF(PairFactory::Expires		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,  256>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,  256>		::insertF(PairFactory::Clone		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,  256>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::PMAllocator	,  512>;
template auto PairVector	<MyAllocator::PMAllocator	,  512>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,  512>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,  512>		::insertF(PairFactory::Normal		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,  512>		::insertF(PairFactory::Expires		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,  512>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,  512>		::insertF(PairFactory::Clone		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	,  512>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::PMAllocator	, 1024>;
template auto PairVector	<MyAllocator::PMAllocator	, 1024>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	, 1024>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	, 1024>		::insertF(PairFactory::Normal		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	, 1024>		::insertF(PairFactory::Expires		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	, 1024>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	, 1024>		::insertF(PairFactory::Clone		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	, 1024>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::PMAllocator	, 2048>;
template auto PairVector	<MyAllocator::PMAllocator	, 2048>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	, 2048>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	, 2048>		::insertF(PairFactory::Normal		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	, 2048>		::insertF(PairFactory::Expires		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	, 2048>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	, 2048>		::insertF(PairFactory::Clone		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	, 2048>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::PMAllocator	, 4096>;
template auto PairVector	<MyAllocator::PMAllocator	, 4096>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	, 4096>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	, 4096>		::insertF(PairFactory::Normal		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	, 4096>		::insertF(PairFactory::Expires		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	, 4096>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	, 4096>		::insertF(PairFactory::Clone		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::PMAllocator	, 4096>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::PMAllocator	&allocator, ListCounter &lc	) -> iterator;







template class PairVector	<MyAllocator::STDAllocator	,    2>;
template auto PairVector	<MyAllocator::STDAllocator	,    2>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    2>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    2>		::insertF(PairFactory::Normal		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    2>		::insertF(PairFactory::Expires		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    2>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    2>		::insertF(PairFactory::Clone		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    2>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::STDAllocator	,    3>;
template auto PairVector	<MyAllocator::STDAllocator	,    3>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    3>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    3>		::insertF(PairFactory::Normal		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    3>		::insertF(PairFactory::Expires		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    3>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    3>		::insertF(PairFactory::Clone		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    3>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::STDAllocator	,    4>;
template auto PairVector	<MyAllocator::STDAllocator	,    4>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    4>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    4>		::insertF(PairFactory::Normal		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    4>		::insertF(PairFactory::Expires		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    4>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    4>		::insertF(PairFactory::Clone		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    4>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::STDAllocator	,    8>;
template auto PairVector	<MyAllocator::STDAllocator	,    8>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    8>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    8>		::insertF(PairFactory::Normal		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    8>		::insertF(PairFactory::Expires		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    8>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    8>		::insertF(PairFactory::Clone		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,    8>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::STDAllocator	,   16>;
template auto PairVector	<MyAllocator::STDAllocator	,   16>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,   16>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,   16>		::insertF(PairFactory::Normal		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,   16>		::insertF(PairFactory::Expires		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,   16>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,   16>		::insertF(PairFactory::Clone		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,   16>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::STDAllocator	,   32>;
template auto PairVector	<MyAllocator::STDAllocator	,   32>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,   32>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,   32>		::insertF(PairFactory::Normal		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,   32>		::insertF(PairFactory::Expires		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,   32>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,   32>		::insertF(PairFactory::Clone		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,   32>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::STDAllocator	,   64>;
template auto PairVector	<MyAllocator::STDAllocator	,   64>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,   64>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,   64>		::insertF(PairFactory::Normal		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,   64>		::insertF(PairFactory::Expires		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,   64>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,   64>		::insertF(PairFactory::Clone		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,   64>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::STDAllocator	,  128>;
template auto PairVector	<MyAllocator::STDAllocator	,  128>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,  128>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,  128>		::insertF(PairFactory::Normal		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,  128>		::insertF(PairFactory::Expires		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,  128>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,  128>		::insertF(PairFactory::Clone		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,  128>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::STDAllocator	,  256>;
template auto PairVector	<MyAllocator::STDAllocator	,  256>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,  256>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,  256>		::insertF(PairFactory::Normal		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,  256>		::insertF(PairFactory::Expires		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,  256>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,  256>		::insertF(PairFactory::Clone		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,  256>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::STDAllocator	,  512>;
template auto PairVector	<MyAllocator::STDAllocator	,  512>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,  512>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,  512>		::insertF(PairFactory::Normal		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,  512>		::insertF(PairFactory::Expires		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,  512>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,  512>		::insertF(PairFactory::Clone		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	,  512>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::STDAllocator	, 1024>;
template auto PairVector	<MyAllocator::STDAllocator	, 1024>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	, 1024>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	, 1024>		::insertF(PairFactory::Normal		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	, 1024>		::insertF(PairFactory::Expires		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	, 1024>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	, 1024>		::insertF(PairFactory::Clone		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	, 1024>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::STDAllocator	, 2048>;
template auto PairVector	<MyAllocator::STDAllocator	, 2048>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	, 2048>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	, 2048>		::insertF(PairFactory::Normal		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	, 2048>		::insertF(PairFactory::Expires		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	, 2048>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	, 2048>		::insertF(PairFactory::Clone		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	, 2048>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::STDAllocator	, 4096>;
template auto PairVector	<MyAllocator::STDAllocator	, 4096>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	, 4096>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	, 4096>		::insertF(PairFactory::Normal		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	, 4096>		::insertF(PairFactory::Expires		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	, 4096>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	, 4096>		::insertF(PairFactory::Clone		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::STDAllocator	, 4096>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::STDAllocator	&allocator, ListCounter &lc	) -> iterator;







template class PairVector	<MyAllocator::ArenaAllocator	,    2>;
template auto PairVector	<MyAllocator::ArenaAllocator	,    2>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    2>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    2>		::insertF(PairFactory::Normal		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    2>		::insertF(PairFactory::Expires		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    2>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    2>		::insertF(PairFactory::Clone		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    2>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::ArenaAllocator	,    3>;
template auto PairVector	<MyAllocator::ArenaAllocator	,    3>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    3>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    3>		::insertF(PairFactory::Normal		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    3>		::insertF(PairFactory::Expires		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    3>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    3>		::insertF(PairFactory::Clone		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    3>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::ArenaAllocator	,    4>;
template auto PairVector	<MyAllocator::ArenaAllocator	,    4>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    4>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    4>		::insertF(PairFactory::Normal		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    4>		::insertF(PairFactory::Expires		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    4>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    4>		::insertF(PairFactory::Clone		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    4>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::ArenaAllocator	,    8>;
template auto PairVector	<MyAllocator::ArenaAllocator	,    8>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    8>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    8>		::insertF(PairFactory::Normal		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    8>		::insertF(PairFactory::Expires		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    8>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    8>		::insertF(PairFactory::Clone		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,    8>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::ArenaAllocator	,   16>;
template auto PairVector	<MyAllocator::ArenaAllocator	,   16>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,   16>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,   16>		::insertF(PairFactory::Normal		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,   16>		::insertF(PairFactory::Expires		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,   16>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,   16>		::insertF(PairFactory::Clone		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,   16>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::ArenaAllocator	,   32>;
template auto PairVector	<MyAllocator::ArenaAllocator	,   32>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,   32>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,   32>		::insertF(PairFactory::Normal		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,   32>		::insertF(PairFactory::Expires		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,   32>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,   32>		::insertF(PairFactory::Clone		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,   32>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::ArenaAllocator	,   64>;
template auto PairVector	<MyAllocator::ArenaAllocator	,   64>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,   64>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,   64>		::insertF(PairFactory::Normal		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,   64>		::insertF(PairFactory::Expires		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,   64>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,   64>		::insertF(PairFactory::Clone		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,   64>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::ArenaAllocator	,  128>;
template auto PairVector	<MyAllocator::ArenaAllocator	,  128>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,  128>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,  128>		::insertF(PairFactory::Normal		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,  128>		::insertF(PairFactory::Expires		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,  128>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,  128>		::insertF(PairFactory::Clone		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,  128>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::ArenaAllocator	,  256>;
template auto PairVector	<MyAllocator::ArenaAllocator	,  256>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,  256>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,  256>		::insertF(PairFactory::Normal		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,  256>		::insertF(PairFactory::Expires		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,  256>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,  256>		::insertF(PairFactory::Clone		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,  256>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::ArenaAllocator	,  512>;
template auto PairVector	<MyAllocator::ArenaAllocator	,  512>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,  512>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,  512>		::insertF(PairFactory::Normal		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,  512>		::insertF(PairFactory::Expires		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,  512>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,  512>		::insertF(PairFactory::Clone		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	,  512>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::ArenaAllocator	, 1024>;
template auto PairVector	<MyAllocator::ArenaAllocator	, 1024>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	, 1024>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	, 1024>		::insertF(PairFactory::Normal		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	, 1024>		::insertF(PairFactory::Expires		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	, 1024>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	, 1024>		::insertF(PairFactory::Clone		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	, 1024>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::ArenaAllocator	, 2048>;
template auto PairVector	<MyAllocator::ArenaAllocator	, 2048>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	, 2048>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	, 2048>		::insertF(PairFactory::Normal		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	, 2048>		::insertF(PairFactory::Expires		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	, 2048>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	, 2048>		::insertF(PairFactory::Clone		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	, 2048>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::ArenaAllocator	, 4096>;
template auto PairVector	<MyAllocator::ArenaAllocator	, 4096>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	, 4096>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	, 4096>		::insertF(PairFactory::Normal		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	, 4096>		::insertF(PairFactory::Expires		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	, 4096>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	, 4096>		::insertF(PairFactory::Clone		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::ArenaAllocator	, 4096>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::ArenaAllocator	&allocator, ListCounter &lc	) -> iterator;







template class PairVector	<MyAllocator::SimulatedArenaAllocator	,    2>;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    2>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    2>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    2>		::insertF(PairFactory::Normal		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    2>		::insertF(PairFactory::Expires		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    2>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    2>		::insertF(PairFactory::Clone		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    2>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::SimulatedArenaAllocator	,    3>;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    3>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    3>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    3>		::insertF(PairFactory::Normal		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    3>		::insertF(PairFactory::Expires		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    3>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    3>		::insertF(PairFactory::Clone		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    3>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::SimulatedArenaAllocator	,    4>;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    4>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    4>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    4>		::insertF(PairFactory::Normal		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    4>		::insertF(PairFactory::Expires		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    4>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    4>		::insertF(PairFactory::Clone		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    4>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::SimulatedArenaAllocator	,    8>;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    8>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    8>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    8>		::insertF(PairFactory::Normal		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    8>		::insertF(PairFactory::Expires		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    8>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    8>		::insertF(PairFactory::Clone		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,    8>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::SimulatedArenaAllocator	,   16>;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   16>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   16>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   16>		::insertF(PairFactory::Normal		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   16>		::insertF(PairFactory::Expires		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   16>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   16>		::insertF(PairFactory::Clone		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   16>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::SimulatedArenaAllocator	,   32>;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   32>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   32>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   32>		::insertF(PairFactory::Normal		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   32>		::insertF(PairFactory::Expires		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   32>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   32>		::insertF(PairFactory::Clone		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   32>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::SimulatedArenaAllocator	,   64>;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   64>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   64>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   64>		::insertF(PairFactory::Normal		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   64>		::insertF(PairFactory::Expires		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   64>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   64>		::insertF(PairFactory::Clone		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,   64>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::SimulatedArenaAllocator	,  128>;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  128>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  128>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  128>		::insertF(PairFactory::Normal		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  128>		::insertF(PairFactory::Expires		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  128>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  128>		::insertF(PairFactory::Clone		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  128>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::SimulatedArenaAllocator	,  256>;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  256>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  256>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  256>		::insertF(PairFactory::Normal		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  256>		::insertF(PairFactory::Expires		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  256>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  256>		::insertF(PairFactory::Clone		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  256>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::SimulatedArenaAllocator	,  512>;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  512>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  512>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  512>		::insertF(PairFactory::Normal		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  512>		::insertF(PairFactory::Expires		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  512>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  512>		::insertF(PairFactory::Clone		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	,  512>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::SimulatedArenaAllocator	, 1024>;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 1024>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 1024>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 1024>		::insertF(PairFactory::Normal		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 1024>		::insertF(PairFactory::Expires		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 1024>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 1024>		::insertF(PairFactory::Clone		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 1024>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::SimulatedArenaAllocator	, 2048>;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 2048>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 2048>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 2048>		::insertF(PairFactory::Normal		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 2048>		::insertF(PairFactory::Expires		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 2048>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 2048>		::insertF(PairFactory::Clone		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 2048>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;



template class PairVector	<MyAllocator::SimulatedArenaAllocator	, 4096>;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 4096>		::find(std::string_view const key, std::true_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 4096>		::find(std::string_view const key, std::false_type	) const -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 4096>		::insertF(PairFactory::Normal		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 4096>		::insertF(PairFactory::Expires		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 4096>		::insertF(PairFactory::Tombstone	&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 4096>		::insertF(PairFactory::Clone		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;
template auto PairVector	<MyAllocator::SimulatedArenaAllocator	, 4096>		::insertF(PairFactory::IFactory		&factory,	MyAllocator::SimulatedArenaAllocator	&allocator, ListCounter &lc	) -> iterator;

} // namespace hm4


