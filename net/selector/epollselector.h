#ifndef NET_EPOLL_SELECTOR_H_
#define NET_EPOLL_SELECTOR_H_

#include "selectordefs.h"

#include <cstdint>

#include <vector>

struct epoll_event;

namespace net{
namespace selector{



class EPollSelector{
public:
	class iterator;

	EPollSelector(uint32_t maxFD);
	EPollSelector(EPollSelector &&other) /* = default */;
	EPollSelector &operator =(EPollSelector &&other) /* = default */;
	~EPollSelector() /* = default */;

	void swap(EPollSelector &other);

	bool insertFD(int fd, FDEvent event = FDEvent::READ);
	bool updateFD(int fd, FDEvent event);
	bool removeFD(int fd);

	WaitStatus wait(int timeout);

	iterator begin() const;
	iterator end() const;

private:
	int				epollFD_;
	std::vector<epoll_event>	fds_;
	int				fdsCount_	= 0;
	uint32_t			fdsConnected_	= 0;
};



class EPollSelector::iterator{
public:
	using hidden_t = epoll_event;

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

