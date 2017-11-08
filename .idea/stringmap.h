#ifndef MY_STRING_MAP_H
#define MY_STRING_MAP_H

#include "stringref.h"

#include <initializer_list>

template<typename T, uint32_t SIZE, T EMPTY>
class StringMap{
public:
	using items_size_type = size_t;

	struct Pair{
		const char	*key;
		T		value;
	};

	constexpr StringMap(const std::initializer_list<Pair>& list){
		clear_();

		for(const Pair &p : list)
			push_(p.key, p.value);
	}

	constexpr const T &operator[](const StringRef &key) const{
		auto const bucket = bucket__(key);

		return values[bucket];
	}

	constexpr auto size(){
		return size_;
	}

	constexpr operator bool() const{
		size_t size = 0;

		for (const auto &x : values)
			if (x != EMPTY)
				++size;

		return size == size_;
	}


	// dangerous to be constexpr
	static bool isEmpty(const T &a){
		return a == EMPTY;
	}

private:
	constexpr void clear_(){
		return fill_(EMPTY);
	}

	constexpr void fill_(const T &value){
		for (auto &x : values)
			x = value;
	}

	constexpr void push_(const StringRef &key, const T &value){
		auto const bucket = bucket__(key);

		auto &element = values[bucket];

		element = value;
		++size_;
	}

	constexpr static uint32_t bucket__(const StringRef &key){
		return key.hash() % SIZE;
	}

private:
	items_size_type	size_		= 0;
	T		values[SIZE]	= {};
};

#endif

