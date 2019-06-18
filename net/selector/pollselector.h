#ifndef NET_POLL_SELECTOR_H_
#define NET_POLL_SELECTOR_H_

#include "selectordefs.h"

#include <vector>

#include "hidden_pointer_iterator.h"

struct pollfd;

namespace net{
namespace selector{

class PollSelector{
public:
	using iterator = hidden_pointer_iterator<pollfd, FDResult>;

	PollSelector(uint32_t maxFD);
	PollSelector(PollSelector &&other) /* = default */;
	PollSelector &operator =(PollSelector &&other) /* = default */;
	~PollSelector() /* = default */;

	bool insertFD(int fd, FDEvent event = FDEvent::READ);
	bool updateFD(int fd, FDEvent event);
	bool removeFD(int fd);

	WaitStatus wait(int timeout);

	iterator begin() const;
	iterator end() const;

private:
	std::vector<pollfd>	fds_;
};



} // namespace selector
} // namespace

#endif

