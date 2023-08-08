#ifndef PAIR_VECTOR_H_
#define PAIR_VECTOR_H_

#include "ilist.h"
#include "listcounter.h"

#include "pairvectorconfig.h"

namespace hm4{

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

		bool destruct(Allocator &allocator) noexcept;

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

		void split(PairVector &other);
		void merge(PairVector &other);

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

		ConstLocateResultPtr locateC_(HPair::HKey const hkey, std::string_view const key) const noexcept;

		auto locateC_(std::string_view const key) const noexcept{
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

		LocateResultPtr locateM_(HPair::HKey const hkey, std::string_view const key) noexcept;

		auto locateM_(std::string_view const key) noexcept{
			return locateM_(HPair::SS::create(key), key);
		}

	public:
		template<bool B>
		iterator find(std::string_view const key, std::bool_constant<B> exact) const noexcept;

		constexpr iterator begin() const noexcept{
			return iterator{ ptr_begin() };
		}

		constexpr iterator end() const noexcept{
			return iterator{ ptr_end() };
		}

	private:
		void assign_(Data *first, Data *last);

	private:
		void zeroing_(){
			size_ = 0;
		}
	};

} // namespace hm4

#endif



