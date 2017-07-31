#ifndef _NET_POLL_SELECTOR_H
#define _NET_POLL_SELECTOR_H

#include "selectordefs.h"

#include <vector>

struct pollfd;

namespace net{
namespace selector{


class PollSelector{
public:
	PollSelector(uint32_t maxFD_);
	PollSelector(PollSelector &&other) /* = default */;
	PollSelector &operator =(PollSelector &&other) /* = default */;
	~PollSelector() /* = default */;

	uint32_t maxFD() const;

	bool insertFD(int fd, FDEvent event = FDEvent::READ);
	bool updateFD(int fd, FDEvent event);
	bool removeFD(int fd);

	WaitStatus wait(int timeout);

	uint32_t getFDStatusCount() const{
		return maxFD();
	}

	FDResult getFDStatus(uint32_t no) const;

private:
	void initializeStatusData_();
	void closeStatusData_();

private:
	std::vector<pollfd>	statusData_;
};


} // namespace selector
} // namespace

#endif

