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

	template<class It, class KData>
	auto binarySearch(It begin, It end, KData const kdata){
		auto comp = [](auto &d, KData const kdata){
			return d.cmp(kdata);
		};

		return ::binarySearch(begin, end, kdata, comp);
	}
} // anonymous namespace

template<class Allocator, size_t Capacity>
bool PairVector<Allocator,Capacity>::destruct(Allocator &allocator) noexcept{
	if (allocator.reset() == false){
		for(auto it = ptr_begin(); it != ptr_end(); ++it){
			using namespace MyAllocator;
			deallocate(allocator, it->pair);
		};
	}

	return true;
}

template<class Allocator, size_t Capacity>
void PairVector<Allocator,Capacity>::assign_(Data *first, Data *last){
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
auto PairVector<Allocator,Capacity>::locateC_(HPair::HKey const hkey, std::string_view const key) const noexcept -> ConstLocateResultPtr{
	assert(!key.empty());

	auto const &[found, it] = binarySearch(ptr_begin(), ptr_end(), KData{ hkey, key } );

	return { found, it };
}

template<class Allocator, size_t Capacity>
auto PairVector<Allocator,Capacity>::locateM_(HPair::HKey const hkey, std::string_view const key) noexcept -> LocateResultPtr{
	assert(!key.empty());

	auto const &[found, it] = binarySearch(ptr_begin(), ptr_end(), KData{ hkey, key } );

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
auto PairVector<Allocator,Capacity>::insertF(HPair::HKey const hkey, PFactory &factory, Allocator &allocator, ListCounter &lc) -> iterator{
	auto const &key = factory.getKey();

	auto [found, it] = locateM_(hkey, key);

	if (found){
		// key exists, overwrite, do not shift

		Pair *olddata = it->pair;

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
		it->pair = newdata.release();

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
bool PairVector<Allocator,Capacity>::erase_(HPair::HKey const hkey, std::string_view const &key, Allocator &allocator, ListCounter &lc){
	auto [found, it] = locateM_(hkey, key);

	if (!found)
		return false;

	lc.dec(it->pair->bytes());

	using namespace MyAllocator;
	deallocate(allocator, it->pair);

	shiftL_(it, ptr_end());

	--size_;

	return true;
}



} // namespace hm4


