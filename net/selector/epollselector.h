#ifndef _NET_EPOLL_SELECTOR_H
#define _NET_EPOLL_SELECTOR_H

#include "selectordefs.h"

#include <cstdint>

#include <vector>

struct epoll_event;

namespace net{
namespace selector{


class EPollSelector{
public:
	EPollSelector(uint32_t maxFD_);
	EPollSelector(EPollSelector &&other) /* = default */;
	EPollSelector &operator =(EPollSelector &&other) /* = default */;
	~EPollSelector() /* = default */;

	void swap(EPollSelector &other);

	uint32_t maxFD() const;

	bool insertFD(int fd, FDEvent event = FDEvent::READ);
	bool updateFD(int fd, FDEvent event);
	bool removeFD(int fd);

	WaitStatus wait(int timeout);

	uint32_t getFDStatusCount() const{
		return statusCount_ < 0 ? 0 : (uint32_t) statusCount_;
	}

	FDResult getFDStatus(uint32_t no) const;

private:
	void initializeEPoll_();
	void closeEPoll_();

	bool mutateFD_(int fd, FDEvent event, int op);

private:
	int				epollFD_;
	std::vector<epoll_event>	statusData_;
	int				statusCount_	= 0;
};


} // namespace selector
} // namespace

#endif

