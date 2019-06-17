#ifndef _NET_EPOLL_SELECTOR_H
#define _NET_EPOLL_SELECTOR_H

#include "selectordefs.h"

#include <cstdint>

#include <vector>

#include <sys/epoll.h>	// struct epoll_event

namespace net{
namespace selector{


class EPollSelector{
public:
	EPollSelector(uint32_t maxFD);
	EPollSelector(EPollSelector &&other) /* = default */;
	EPollSelector &operator =(EPollSelector &&other) /* = default */;
	~EPollSelector() /* = default */;

	void swap(EPollSelector &other);

	bool insertFD(int fd, FDEvent event = FDEvent::READ);
	bool updateFD(int fd, FDEvent event);
	bool removeFD(int fd);

	WaitStatus wait(int timeout);

	auto begin() const{
		return std::begin(fds_);
	}

	auto end() const{
		return std::begin(fds_) + fdsCount_;
	}

	static FDResult getFDStatus(epoll_event const &p);

private:
	int				epollFD_;
	std::vector<epoll_event>	fds_;
	int				fdsCount_	= 0;
};


} // namespace selector
} // namespace

#endif

