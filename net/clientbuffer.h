#ifndef _NET_CONNECTION_H
#define _NET_CONNECTION_H

#include "iobuffer.h"
#include "mytime.h"


namespace net{


class ClientBuffer : public IOBuffer{
private:
	MyTimer timer_;

public:
	bool expired(uint32_t const timeout) const noexcept{
		return timer_.expired(timeout);
	}

	void restartTimer() noexcept{
		return timer_.restart();
	}
};


} // namespace

#endif

