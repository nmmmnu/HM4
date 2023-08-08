#ifndef PAIR_VECTOR_H_
#define PAIR_VECTOR_H_

#include "ilist.h"
#include "listcounter.h"

#include "pairvectorconfig.h"

#include "binarysearch.h"

#include "ilist_updateinplace.h"

#include <cassert>
#include <cstring>		// memmove
#include <stdexcept>		// std::bad_alloc

namespace hm4{

	namespace pairvector_impl_{

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
		auto myBinarySearch(It begin, It end, KData const kdata){
			auto comp = [](auto &d, KData const kdata){
				return d.cmp(kdata);
			};

			return ::binarySearch(begin, end, kdata, comp, 0u);
		}

	} // pairvector_impl_

	template<class Allocator, size_t Capacity = 1024>
	class PairVector{
		constexpr static size_t capacity__ = Capacity;
	public:
		using size_type			= config::size_type;
		using difference_type		= config::difference_type;

		using const_ptr_iterator	= PairVectorConfig::const_ptr_iterator;
		using ptr_iterator		= PairVectorConfig::ptr_iterator;

		using iterator			= PairVectorConfig::iterator;

		using Data			= PairVectorConfig::Data;

	private:
		size_type	size_			= 0;
		Data		data_[capacity__];

	public:
		// (Manual) c-tor

		void construct() noexcept{
			return zeroing_();
		}

		// (Manual) d-tor

		bool destruct(Allocator &allocator) noexcept{
			if (allocator.reset() == false){
				for(auto it = ptr_begin(); it != ptr_end(); ++it){
					using namespace MyAllocator;
					deallocate(allocator, it->pair);
				};
			}

			return true;
		}

		void clear(Allocator &allocator) noexcept{
			destruct(allocator);
			zeroing_();
		}

	public:
		constexpr size_type size() const noexcept{
			return size_;
		}

		constexpr size_type capacity() const noexcept{
			return capacity__;
		}

		constexpr bool full() const noexcept{
			return size() == capacity();
		}

		constexpr Pair const &operator[](size_type const index) const noexcept{
			return *data_[index];
		}

		constexpr Data const &frontData() const noexcept{
			return data_[0];
		}

		constexpr Data const &backData() const noexcept{
			return data_[size_ - 1];
		}

		constexpr Pair const &front() const noexcept{
			return *frontData();
		}

		constexpr Pair const &back() const noexcept{
			return *backData();
		}

		template<class PFactory>
		iterator insertF(HPair::HKey const hkey, PFactory &factory, Allocator &allocator, ListCounter &lc);

		bool erase_(HPair::HKey const hkey, std::string_view const &key, Allocator &allocator, ListCounter &lc);

		void split(PairVector &other){
			assert(other.size() == 0);

			auto const len = size_ / 2;

			other.assign_(ptr_begin() + len, ptr_end());

			size_ -= other.size();
		}

		void merge(PairVector &other){
			assign_(other.ptr_begin(), other.ptr_end());

			other.size_ = 0;
		}

	public:
		// used for testing
		template<class PFactory>
		iterator xInsertF(PFactory &factory, Allocator &allocator, ListCounter &lc){
			auto const hkey = HPair::SS::create(factory.getKey());
			return insertF(hkey, factory, allocator, lc);
		}

		// used for testing
		template<class PFactory>
		iterator xInsertF(PFactory &factory, Allocator &allocator){
			ListCounter lc;
			return xInsertF(factory, allocator, lc);
		}

		// used for testing
		bool xErase_(std::string_view const &key, Allocator &allocator, ListCounter &lc){
			auto const hkey = HPair::SS::create(key);
			return erase_(hkey, key, allocator, lc);
		}

		// used for testing
		bool xErase_(std::string_view const &key, Allocator &allocator){
			ListCounter lc;
			return xErase_(key, allocator, lc);
		}

	public:
		struct ConstLocateResultPtr{
			bool			found;
			const_ptr_iterator	it;
		};

		const_ptr_iterator ptr_begin() const noexcept{
			return data_;
		}

		const_ptr_iterator ptr_end() const noexcept{
			return data_ + size_;
		}

		ConstLocateResultPtr locateC_(HPair::HKey const hkey, std::string_view const key) const noexcept{
			assert(!key.empty());

			using PairVectorConfig::KData;

			using namespace pairvector_impl_;

			auto const &[found, it] = myBinarySearch(ptr_begin(), ptr_end(), KData{ hkey, key } );

			return { found, it };
		}

		auto xLocateC_(std::string_view const key) const noexcept{
			return locateC_(HPair::SS::create(key), key);
		}

	public:
		struct LocateResultPtr{
			bool			found;
			ptr_iterator		it;
		};

		ptr_iterator ptr_begin() noexcept{
			return data_;
		}

		ptr_iterator ptr_end() noexcept{
			return data_ + size_;
		}

		LocateResultPtr locateM_(HPair::HKey const hkey, std::string_view const key) noexcept{
			assert(!key.empty());

			using PairVectorConfig::KData;

			using namespace pairvector_impl_;

			auto const &[found, it] = myBinarySearch(ptr_begin(), ptr_end(), KData{ hkey, key } );

			return { found, it };
		}

		auto xLocateM_(std::string_view const key) noexcept{
			return locateM_(HPair::SS::create(key), key);
		}

	public:
		template<bool ExactMatch>
		iterator xFind(std::string_view const key, std::bool_constant<ExactMatch>) const noexcept{
			auto const &[found, it] = xLocateC_(key);

			if constexpr(ExactMatch)
				return found ? iterator{ it } : end();
			else
				return iterator{ it };
		}

		constexpr iterator begin() const noexcept{
			return iterator{ ptr_begin() };
		}

		constexpr iterator end() const noexcept{
			return iterator{ ptr_end() };
		}

	private:
		void assign_(Data *first, Data *last){
			auto const len = static_cast<size_type>(last - first);

			if (size() + len > capacity())
				throw std::bad_alloc{};

			std::move(first, last, ptr_end());

			size_ += len;
		}

	private:
		void zeroing_(){
			size_ = 0;
		}
	};





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
		pairvector_impl_::shiftR_(it, ptr_end());

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

		pairvector_impl_::shiftL_(it, ptr_end());

		--size_;

		return true;
	}

} // namespace hm4

#endif



