#ifndef NET_SIMPLE_SPARE_POOL_H
#define NET_SIMPLE_SPARE_POOL_H

#include <algorithm> // make_heap
#include "iobuffer.h"

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

		friend bool operator<(S const &a, S const &b){
			auto _ = [](S const &a){
				return a.data.capacity();
			};

			return _(a) < _(b);
		}
	};

public:
	HeapSparePool(uint32_t conf_minSparePoolSize, uint32_t conf_maxSparePoolSize, size_t conf_bufferCapacity) :
								conf_minSparePoolSize_	(conf_minSparePoolSize	),
								conf_maxSparePoolSize_	(conf_maxSparePoolSize	),
								conf_bufferCapacity_	(conf_bufferCapacity	){

		sparePool_.reserve(conf_maxSparePoolSize_ + 1);

		for(size_t i = 0; i < conf_minSparePoolSize; ++i)
			sparePool_.emplace_back(construct_());
	}

	bool empty() const{
		return sparePool_.empty();
	}

	auto size() const{
		return sparePool_.size();
	}

public:
	data_type pop(){
		if (! empty()){
			return popMax_();
		}else{
			return construct_();
		}
	}

	void push(data_type &&item){
		push_(std::move(item));

		if (size() > conf_maxSparePoolSize_)
			popLeaf_<1>();
	}

	void balance(){
		if (size() > conf_minSparePoolSize_){
			popLeaf_<0>();
		}else
		if (size() < conf_minSparePoolSize_){
			construct_back_();
		}
	}

private:
	void construct_back_(){
		push(construct_());
	}

	data_type construct_(){
		data_type x;
		x.reserve(conf_bufferCapacity_);
		return x;
	}

private:
	data_type popMax_(){
		std::pop_heap(std::begin(sparePool_), std::end(sparePool_));
		data_type item = std::move(sparePool_.back().data);
		sparePool_.pop_back();

		log__("get", item.capacity());

		return item;
	}

	void push_(data_type &&item){
		auto const capacity = item.capacity();

		sparePool_.emplace_back(std::move(item));
		std::push_heap(std::begin(sparePool_), std::end(sparePool_));

		log__("adding", capacity);
	}

	template<bool B>
	void popLeaf_(){
		auto const capacity = sparePool_.back().data.capacity();

		sparePool_.pop_back();

		if constexpr(B)
			log__("remove after add",    capacity);
		else
			log__("remove when balance", capacity);
	}

private:
	void log__(const char *s, size_t const capacity) const{
		if constexpr(0){
			printf("%s %zu\n", s, capacity);

			if constexpr(1){
				printf("Buffer Heap:\n");
				for(auto const &x : sparePool_)
					printf("- %10zu %p\n", x.data.capacity(), (void *) x.data.data());
				printf("%5zu items total:\n", sparePool_.size());
			}
		}
	}

private:
	uint32_t	conf_minSparePoolSize_	;
	uint32_t	conf_maxSparePoolSize_	;
	size_t		conf_bufferCapacity_	;

	std::vector<S>	sparePool_;
};

} // namespace

#endif

