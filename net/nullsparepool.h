#ifndef NULL_SPARE_POOL_H
#define NULL_SPARE_POOL_H

#include "iobuffer.h"

namespace net{

struct NullSparePool{
	using data_type = IOBuffer::container_type;

	constexpr NullSparePool(uint32_t, uint32_t, size_t conf_bufferCapacity) :
					conf_bufferCapacity_(conf_bufferCapacity){
	}

	auto pop() const{
		data_type x;
		x.reserve(conf_bufferCapacity_);
		return x;
	}

	constexpr static bool empty(){
		return true;
	}

	constexpr static size_t size(){
		return 0;
	}

	template<typename T>
	constexpr static void push(T &&){
	}

	constexpr static void balance(){
	}

private:
	size_t				conf_bufferCapacity_	;
};

} // namespace

#endif

