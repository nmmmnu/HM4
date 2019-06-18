#ifndef NET_KQUEUE_SELECTOR_H_
#define NET_KQUEUE_SELECTOR_H_

#include "selectordefs.h"

#include <vector>

struct kevent;

namespace net{
namespace selector{


class KQueueSelector{
public:
	class iterator;

	KQueueSelector(uint32_t maxFD_);
	KQueueSelector(KQueueSelector &&other) /* = default */;
	KQueueSelector &operator =(KQueueSelector &&other) /* = default */;
	~KQueueSelector() /* = default */;

	void swap(KQueueSelector &other);

	bool insertFD(int fd, FDEvent event = FDEvent::READ);

	bool updateFD(int const fd, FDEvent const event){
		removeFD(fd);
		return insertFD(fd, event);
	}

	bool removeFD(int fd);

	WaitStatus wait(int timeout);

	iterator begin() const;
	iterator end() const;

private:
	int				kqueueFD_;
	std::vector<struct kevent>	fds_;
	int				fdsCount_	= 0;
	uint32_t			fdsConnected_	= 0;
};



class KQueueSelector::iterator{
public:
	using hidden_t = struct kevent;

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

