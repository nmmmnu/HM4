#ifndef LIST_COUNTER_H_
#define LIST_COUNTER_H_

#include "ilist.h"

namespace hm4{

	class ListCounter{
	public:
		using size_type = config::size_type;

	public:
		auto size() const{
			return dataCount_;
		}

		auto bytes() const{
			return dataSize_;
		}

	public:
		void clr(){
			dataCount_	= 0;
			dataSize_	= 0;
		}

		void inc(size_t const s){
			++dataCount_;
			dataSize_ += s;
		}

		void dec(size_t const s){
			--dataCount_;
			dataSize_ -= s;
		}

		void upd(size_t const so, size_t const sn){
			dataSize_ = dataSize_ - so + sn;
		}

	private:
		size_type	dataCount_	= 0;
		size_t		dataSize_	= 0;
	};

} // namespace

#endif

