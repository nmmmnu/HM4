#ifndef TOP_HEAP_H_
#define TOP_HEAP_H_

#include <algorithm>
#include <functional>

#include <cassert>

#include "staticvector.h"

namespace top_heap{

	template<typename T, size_t Size, typename Compare>
	struct TopKHeap{
		static_assert(Size > 0);

		constexpr explicit TopKHeap(size_t capacity) : capacity_(capacity){
			assert(capacity > 0);

			data_.reserve(capacity);
		}

		constexpr TopKHeap() : TopKHeap(Size){}

		constexpr bool push(T value){
			if (data_.size() < capacity_){
				data_.push_back(std::move(value));

				std::push_heap(std::begin(data_), std::end(data_), comp__());

				return true;
			}

			if (comp__()(data_.front(), value)){
				std::pop_heap (std::begin(data_), std::end(data_), comp__());

				data_.back() = std::move(value);

				std::push_heap(std::begin(data_), std::end(data_), comp__());

				return true;
			}

			return false;
		}

		constexpr T pop(){
			assert(size() > 0);

			std::pop_heap(std::begin(data_), std::end(data_), comp__());

			T value = std::move(data_.back());

			data_.pop_back();

			return value;
		}

		constexpr auto size() const{
			return data_.size();
		}

		constexpr bool empty() const{
			return size() == 0;
		}

		constexpr const auto &data() const{
			return data_;
		}

		constexpr auto &data(){
			return data_;
		}

	private:
		constexpr static auto comp__(){
			return Compare{};
		}

	private:
		size_t			capacity_;
		StaticVector<T, Size>	data_;
	};

	template<typename T, size_t Size>
	using TopKSmallest = TopKHeap<T, Size, std::greater<T> >;

	template<typename T, size_t Size>
	using TopKLargest  = TopKHeap<T, Size, std::less<T> >;

} // namespace top_heap

#endif

