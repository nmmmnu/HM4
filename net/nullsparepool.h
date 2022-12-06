#ifndef NET_SIMPLE_SPARE_POOL_H
#define NET_SIMPLE_SPARE_POOL_H

namespace net{

struct SimpleSparePool{
	constexpr SimpleSparePool(uint32_t, uint32_t, size_t conf_bufferCapacity) :
					conf_bufferCapacity_(conf_bufferCapacity){
	}

	constexpr auto pop() const{
		data_type x;
		x.reserve(conf_bufferCapacity_);
		return x;
	}

	constexpr static bool empty(){
		return true;
	}

	constexpr static bool size(){
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

