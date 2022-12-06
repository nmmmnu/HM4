#ifndef NET_SIMPLE_SPARE_POOL_H
#define NET_SIMPLE_SPARE_POOL_H

#include <vector>
#include "iobuffer.h"

namespace net{

class SimpleSparePool{
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

	using SparePoolContainer	= std::vector<S>;

public:
	SimpleSparePool(uint32_t conf_minSparePoolSize, uint32_t conf_maxSparePoolSize, size_t conf_bufferCapacity) :
								conf_minSparePoolSize_	(conf_minSparePoolSize	),
								conf_maxSparePoolSize_	(conf_maxSparePoolSize	),
								conf_bufferCapacity_	(conf_bufferCapacity	){

		sparePool_.reserve (conf_maxSparePoolSize_);

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
			auto x = std::move(sparePool_.back().data);

			sparePool_.pop_back();

			return x;
		}else{
			return construct_();
		}
	}

	void push(data_type &&item){
		if (size() < conf_maxSparePoolSize_)
			sparePool_.push_back(std::move(item));
	}

	void balance(){
		if (size() > conf_minSparePoolSize_){
			pop_back();
		}else
		if (size() < conf_minSparePoolSize_){
			emplace_back();
		}
	}

private:
	void pop_back(){
		sparePool_.pop_back();
	}

	void emplace_back(){
		sparePool_.emplace_back(conf_bufferCapacity_);
	}

	data_type construct_(){
		data_type x;
		x.reserve(conf_bufferCapacity_);
		return x;
	}

private:
	uint32_t			conf_minSparePoolSize_	;
	uint32_t			conf_maxSparePoolSize_	;
	size_t				conf_bufferCapacity_	;

	SparePoolContainer		sparePool_;
};

} // namespace

#endif

