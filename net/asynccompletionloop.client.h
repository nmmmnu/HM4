#ifndef _NET_ASYNC_LOOP_CLIENT_H_
#define _NET_ASYNC_LOOP_CLIENT_H_

#include "iobuffer.h"
#include "mytime.h"


namespace net{

	struct Client{
		IOBuffer	buffer;
		const char	*offcet = nullptr;

		Client(size_t conf_buffer_spare_pool) : buffer(conf_buffer_spare_pool){}
		Client(IOBuffer::container_type &&b) : buffer(std::move(b)){}
	};

} // namespace net

#endif

