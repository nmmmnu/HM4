#ifndef LEVEL_ORDER_LOOKUP_H
#define LEVEL_ORDER_LOOKUP_H

#include <cstdint>
#include <cassert>

template<typename level_type, level_type LEVELS, typename size_type, size_type SIZE>
class LevelOrderLookupFactory;

template<uint8_t LEVELS>
class LevelOrderLookup{
public:
	using level_type = uint8_t;

	using size_type = uint16_t;

	using Iterator = const size_type*;

private:
	constexpr static size_type SIZE = (1 << LEVELS) - 1;

	using MyFactory = LevelOrderLookupFactory<level_type, LEVELS, size_type, SIZE>;

public:
	constexpr LevelOrderLookup() : value(){
		MyFactory::build(value);
	}

	constexpr level_type levels() const{
		return LEVELS;
	}

	constexpr size_type size() const{
		return SIZE;
	}

	constexpr size_type operator[](size_type const index) const{
		assert(index < SIZE);
		return value[index];
	}

	constexpr Iterator begin() const{
		return value;
	}

	constexpr Iterator end() const{
		return value + SIZE;
	}

private:
	size_type value[SIZE];

};

// ==============================

template<typename level_type, level_type LEVELS, typename size_type, size_type SIZE>
class LevelOrderLookupFactory{
public:
	constexpr static void build(size_type *value){
		LevelOrderLookupFactory factory(value);
		factory.reorder_();
	}

private:
	constexpr LevelOrderLookupFactory(size_type *value) : value(value){}

	constexpr void reorder_(){
		for(level_type level = 0; level < LEVELS; ++level)
			reorderLevel_(level);
	}

	constexpr void reorderLevel_(level_type const target_level){
		reorder_(0, SIZE, target_level, 0);
	}

	constexpr void reorder_(size_type const begin, size_type const end, level_type const target_level, level_type const current_level){
		if (begin >= end){
			// size is guaranteed to be "aligned",
			// no push_(NIL) needed...
			return;
		}

		size_type const mid = size_type( begin + ((end - begin) >> 1) );

		if (current_level == target_level)
			return push_back(mid);

		if (current_level < target_level){
			level_type const next_level = level_type(current_level + 1);

			reorder_(begin,               mid, target_level, next_level);
			reorder_(level_type(mid + 1), end, target_level, next_level);
		}
	}

private:
	constexpr void push_back(size_type const data){
		value[push_pos_++] = data;
	}

private:
	size_type	*value;
	size_type	push_pos_	= 0;
};

#endif

