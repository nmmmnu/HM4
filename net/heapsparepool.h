#ifndef NET_SIMPLE_SPARE_POOL_H
#define NET_SIMPLE_SPARE_POOL_H

#include "iobuffer.h"
#include "minmaxheap.h"

namespace net{

class HeapSparePool{
	using data_type = IOBuffer::container_type;

	struct S{
		data_type data;

		S(size_t capacity){
			data.reserve(capacity);
		}

		S(data_type &&data) : data(std::move(data)){}

		S(S &&other)			= default;
		S &operator=(S &&other)		= default;

		S(S const &other)		= delete;
		S &operator=(S const &other)	= delete;
	};

	struct Comp{
		static auto _(S const &a){
			return a.data.capacity();
		}

		static bool less(S const &a, S const &b){
			return _(a) < _(b);
		}

		static bool greater(S const &a, S const &b){
			return _(a) > _(b);
		}
	};

	using SparePoolContainer = MinMaxHeap<S, std::vector<S>, Comp>;

public:
	HeapSparePool(uint32_t conf_minSparePoolSize, uint32_t conf_maxSparePoolSize, size_t conf_bufferCapacity) :
								conf_minSparePoolSize_	(conf_minSparePoolSize		),
								conf_maxSparePoolSize_	(conf_maxSparePoolSize		),
								conf_bufferCapacity_	(conf_bufferCapacity		){

		for(size_t i = 0; i < conf_minSparePoolSize; ++i)
			emplace_back();
	}

	bool empty() const{
		return sparePool_.empty();
	}

	auto size() const{
		return sparePool_.size();
	}

public:
	auto pop(){
		if (! sparePool_.empty()){
			auto x = sparePool_.popMax();
			log__("get", x);
			return std::move(x.data);
		}else{
			return construct_();
		}
	}

	void push(data_type &&item){
		log__("adding", item);
		sparePool_.insert(std::move(item));

		if (size() > conf_maxSparePoolSize_){
			auto x = /* destruct */ sparePool_.popMin();
			log__("add_remove", x);
		}
	}

	void balance(){
		if (size() > conf_minSparePoolSize_){
			auto x = /* destruct */  sparePool_.popMin();
			log__("remove", x);
		}else
		if (size() < conf_minSparePoolSize_){
			emplace_back();
		}
	}

private:
	void emplace_back(){
		sparePool_.insert(conf_bufferCapacity_);
	}

	data_type construct_(){
		data_type x;
		x.reserve(conf_bufferCapacity_);
		return x;
	}

private:
	static void log__(const char *s, data_type const &data){
		if constexpr(0)
			printf("%s %zu\n", s, data.capacity());
	}

	static void log__(const char *s, S const &x){
		return log__(s, x.data);
	}

private:
	uint32_t			conf_minSparePoolSize_	;
	uint32_t			conf_maxSparePoolSize_	;
	size_t				conf_bufferCapacity_	;

	SparePoolContainer		sparePool_{ conf_maxSparePoolSize_ + 1 };
};

} // namespace

#endif

