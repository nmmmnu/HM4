#ifndef _NET_ASYNC_LOOP_CLIENT_H_
#define _NET_ASYNC_LOOP_CLIENT_H_

#include "iobuffer.h"
#include "mytime.h"


namespace net{

	template <typename MemoryManager>
	struct Client{
		IOBuffer<MemoryManager>	buffer;
		MyTimer 		timer;

		Client(MemoryManager &mm) : buffer(mm){}

	//	Client(Client &&b) : buffer(std::move(b)){}
	};

} // namespace net

#endif

