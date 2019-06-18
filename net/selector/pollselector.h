#ifndef NET_POLL_SELECTOR_H_
#define NET_POLL_SELECTOR_H_

#include "selectordefs.h"

#include <vector>

struct pollfd;

namespace net{
namespace selector{


class PollSelector{
public:
	class iterator;

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



class PollSelector::iterator{
public:
	using hidden_t = pollfd;

	iterator(const hidden_t *pos) : pos(pos){}

	bool operator ==(iterator const &other) const;
	bool operator !=(iterator const &other) const;
	iterator &operator ++();
	FDResult operator *() const;

private:
	const hidden_t *pos;
};



} // namespace selector
} // namespace

#endif

