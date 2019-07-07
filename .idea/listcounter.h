#ifndef LIST_COUNTER_H_
#define LIST_COUNTER_H_

#include "ilist.h"

//#include "myalign.h"

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

		auto bytesAligned() const{
		//	return dataSizeAligned_;
		}

	public:
		void clr(){
			dataCount_		= 0;
			dataSize_		= 0;
		//	dataSizeAligned_	= 0;
		}

		void inc(size_t const s){
			++dataCount_;
			dataSize_		+= s;
		//	dataSizeAligned_	+= calcAlign__(s);
		}

		void dec(size_t const s){
			--dataCount_;

			dataSize_		-= s;
		//	dataSizeAligned_	-= calcAlign__(s);
		}

		void upd(size_t const so, size_t const sn){
			dataSize_		= dataSize_
							- so
							+ sn
			;

		//	dataSizeAligned_	= dataSizeAligned_
		//					- calcAlign__(so)
		//					+ calcAlign__(sn)
		//	;
		}

	private:
		//static size_t calcAlign__(size_t const size){
		//	return my_align::calc(size, PairConf::ALIGN);
		//}

	private:
		size_type	dataCount_		= 0;
		size_t		dataSize_		= 0;
	//	size_t		dataSizeAligned_	= 0;
	};

} // namespace

#endif

