#ifndef SORTED_ARRAY_H_
#define SORTED_ARRAY_H_

#include <stdexcept>		// std::bad_alloc
#include <type_traits>
#include <initializer_list>
#include <cstring>		// memmove

#include "binarysearch.h"

namespace sorted_vector_impl_{
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

	template<typename T>
	struct construct_none{
		constexpr construct_none(T const &value) : value(value){}

		constexpr bool operator()(T &x) const{
			x = value;
			return true;
		}

	private:
		T const &value;
	};

	struct destruct_none{
		template<typename T>
		constexpr void operator()(T &) const{
		}
	};
}

template<typename T, size_t Size>
class SortedVector{
	static_assert(
		std::is_trivially_copyable_v		<T>	&&
		std::is_trivially_copy_constructible_v	<T>	&&
		std::is_trivially_move_constructible_v	<T>	&&
		std::is_trivially_destructible_v	<T>	&&
		std::is_standard_layout_v		<T>	&&

		true,	"T must be POD-like type"
	);

public:
	using value_type	= T;
	using size_type		= size_t;

	using iterator		=       T*;
	using const_iterator	= const T*;

private:
	size_type	size_	= 0;

	T		buffer_[Size]{};

public:
	constexpr SortedVector() = default;

	template<class Iterator>
	constexpr SortedVector(Iterator begin, Iterator end){
		assign_(begin, end);
	}

	template<class Container>
	constexpr SortedVector(Container const &container) :
		SortedVector(std::begin(container), std::end(container)){}

	constexpr SortedVector(std::initializer_list<T> const &container) :
		SortedVector(std::begin(container), std::end(container)){}

	// MISC

	constexpr
	void clear() noexcept{
		size_ = 0;
	}

	// ITERATORS

	constexpr
	iterator begin() noexcept{
		return data();
	}

	constexpr
	iterator end() noexcept{
		return data() + size();
	}

	// CONST ITERATORS

	constexpr const_iterator begin() const noexcept{
		return data();
	}

	constexpr const_iterator end() const noexcept{
		return data() + size();
	}

	// C++11 CONST ITERATORS

	constexpr const_iterator cbegin() const noexcept{
		return begin();
	}

	constexpr const_iterator cend() const noexcept{
		return end();
	}

	// Size

	constexpr size_type size() const noexcept{
		return size_;
	}

	constexpr bool empty() const noexcept{
		return size() == 0;
	}

	// MORE Size

	constexpr size_type capacity() const noexcept{
		return Size;
	}

	constexpr size_type max_size() const noexcept{
		return Size;
	}

	// DATA

	constexpr
	value_type *data() noexcept{
		return buffer_;
	}

	constexpr const value_type *data() const noexcept{
		return buffer_;
	}

	// ACCESS DIRECTLY

	template<typename Key, class Comp, bool ExactMatch>
	const_iterator search(Key const &key, Comp comp, std::bool_constant<ExactMatch> exact) const noexcept{
		return binarySearchFix_(key, comp, exact);
	}

	template<typename Key, bool ExactMatch>
	const_iterator search(Key const &key, std::bool_constant<ExactMatch> exact) const noexcept{
		return binarySearchFix_(key, exact);
	}

	// OPERATOR []

	constexpr const value_type &operator[](size_type const index) const noexcept{
		return data()[index];
	}

	// FRONT

	constexpr const value_type &front() const noexcept{
		return data()[0];
	}

	// BACK

	constexpr const value_type &back() const noexcept{
		return data()[size_ - 1];
	}

	// MUTATIONS

private:
	template<class Construct, class Destruct>
	iterator replace_(iterator it, Construct &&construct, Destruct &&destruct){
		T x;

		if (construct(x)){
			destruct(*it);

			*it = std::move(x);
		}

		return it;
	}

public:
	template<typename Key, class Construct, class Destruct>
	iterator insert(Key const &key, Construct &&construct, Destruct &&destruct){
		auto [found, it] = binarySearch_(key);

		if (found){
			return replace_(
				it,
				std::forward<Construct	>(construct	),
				std::forward<Destruct	>(destruct	)
			);
		}

		if (size() == capacity())
			throw std::bad_alloc{};

		T x;

		if (construct(x)){
			using namespace sorted_vector_impl_;

			// make space
			shiftR_(it, end());
			++size_;

			destruct(*it);

			*it = std::move(x);
		}

		return it;
	}

	auto insert(T const &key){
		using namespace sorted_vector_impl_;

		return insert(key, construct_none{ key }, destruct_none{});
	}

	template<typename Key, class Destruct>
	void erase(Key const &key, Destruct &&destruct){
		using namespace sorted_vector_impl_;

		auto [found, it] = binarySearch_(key);

		if (!found)
			return;

		destruct(*it);

		shiftL_(it, end());
		--size_;
	}

	auto erase(T const &key){
		using namespace sorted_vector_impl_;

		return erase(key, destruct_none{});
	}

public:
	template<class Iterator>
	constexpr void assign_(Iterator first, Iterator last){
		auto const len = static_cast<size_type>(last - first);

		if (size() + len > capacity())
			throw std::bad_alloc{};

		{
			auto dest = end();
			for (auto it = first; it != last; ++it, ++dest)
				*dest = *it;
		}

		size_ += len;
	}

	template<size_t S>
	void split(SortedVector<T, S> &other){
		auto const len = size() / 2;

		other.assign_(begin() + len, end());

		size_ -= len;
	}

	template<size_t S>
	void merge(SortedVector<T, S> const &other){
		assign_(std::begin(other), std::end(other));
	}

private:
	template<typename Key, class Comp>
	auto binarySearch_(Key const &key, Comp &&comp) noexcept{
		return binarySearch(begin(), end(), key, std::forward<Comp>(comp));
	}

	template<typename Key>
	auto binarySearch_(Key const &key) noexcept{
		return binarySearch(begin(), end(), key);
	}

	// ---------------------

	template<typename Key, class Comp>
	auto binarySearch_(Key const &key, Comp &&comp) const noexcept{
		return binarySearch(begin(), end(), key, comp);
	}

	template<typename Key>
	auto binarySearch_(Key const &key) const noexcept{
		return binarySearch(begin(), end(), key);
	}

	// ---------------------

	template<bool ExactMatch>
	constexpr const_iterator binarySearchFixer_(BinarySearchResult<const_iterator> const &bsr, std::bool_constant<ExactMatch>) const noexcept{
		auto const &[found, it] = bsr;

		if constexpr(ExactMatch)
			return found ? it : end();
		else
			return it;
	}

	// ---------------------

	template<typename Key, class Comp, bool ExactMatch>
	auto binarySearchFix_(Key const &key, Comp comp, std::bool_constant<ExactMatch> exact) const noexcept{
		return binarySearchFixer_(
			binarySearch(begin(), end(), key, comp),
			exact
		);
	}

	template<typename Key, bool ExactMatch>
	auto binarySearchFix_(Key const &key, std::bool_constant<ExactMatch> exact) const noexcept{
		return binarySearchFixer_(
			binarySearch(begin(), end(), key),
			exact
		);
	}
};

#endif



