#ifndef _NET_ASYNC_LOOP_CLIENT_H_
#define _NET_ASYNC_LOOP_CLIENT_H_

#include "iobuffer.h"
//#include "mytime.h"


namespace net{

	template<typename MemoryManager>
	struct Client{
		IOBuffer<MemoryManager>	buffer;
		const char		*offcet = nullptr;

		using Container = typename IOBuffer<MemoryManager>::container_type;

		Client(MemoryManager &mm) : buffer(mm){}

	//	Client(Container &&b) : buffer(std::move(b)){}
	};

} // namespace net

#endif

