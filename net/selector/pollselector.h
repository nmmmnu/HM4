#ifndef _NET_POLL_SELECTOR_H
#define _NET_POLL_SELECTOR_H

#include "selectordefs.h"

#include <vector>

#include <poll.h>	// struct pollfd

namespace net{
namespace selector{


class PollSelector{
public:
	PollSelector(uint32_t maxFD);
	PollSelector(PollSelector &&other) /* = default */;
	PollSelector &operator =(PollSelector &&other) /* = default */;
	~PollSelector() /* = default */;

	bool insertFD(int fd, FDEvent event = FDEvent::READ);
	bool updateFD(int fd, FDEvent event);
	bool removeFD(int fd);

	WaitStatus wait(int timeout);

	auto begin() const{
		return std::begin(fds_);
	}

	auto end() const{
		return std::end(fds_);
	}

	static FDResult getFDStatus(pollfd const &p);

private:
	std::vector<pollfd>	fds_;
};


} // namespace selector
} // namespace

#endif

