#ifndef MY_STRING_MAP_H
#define MY_STRING_MAP_H

#include "stringref.h"

#include <initializer_list>

template<typename T, uint32_t SIZE, T ZERO>
class StringMap{
public:
	using size_type = uint32_t;

	struct Pair{
		const char	*key;
		T		value;
	};

	constexpr StringMap(const std::initializer_list<Pair>& list){
		for (auto &x : values)
			x = ZERO;

		for(const Pair &p : list)
			push_(p.key, p.value);
	}

	constexpr bool collisions() const{
		return collisions_;
	}

	constexpr const T &operator[](const StringRef &key) const{
		auto const bucket = bucket__(key);

		return values[bucket];
	}

	constexpr static bool isZero(const T &a){
		return a == ZERO;
	}

private:
	constexpr void push_(const StringRef &key, const T &value){
		auto const bucket = bucket__(key);

		auto &element = values[bucket];

		if (element != ZERO)
			collisions_ = true;

		element = value;
	}

	constexpr static size_type bucket__(const StringRef &key){
		return hash__(key) % SIZE;
	}

private:
	constexpr static uint32_t hash__(const StringRef &key){
		constexpr uint32_t djb_start = 5381;

		auto hash = djb_start;

		for(const char &c : key)
			hash = ((hash << 5) + hash) + c;

		return hash;
	}

private:
	bool	collisions_	= false;
	T	values[SIZE]	= {};
};

#endif

