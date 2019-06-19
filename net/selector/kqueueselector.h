#ifndef NET_KQUEUE_SELECTOR_H_
#define NET_KQUEUE_SELECTOR_H_

#include "selectordefs.h"

#include <vector>

struct kevent;

namespace hpi{
	using hidden_t = kevent;
	using net::selector::FDResult;

	bool eq(const hidden_t *a, const hidden_t *b)	__attribute__((const))	;
	void inc(const hidden_t * &a)						;
	FDResult conv(const hidden_t *a)		__attribute__((const))	;
}

#include "hidden_pointer_iterator.h"

namespace net{
namespace selector{


class KQueueSelector{
public:
	using iterator = hidden_pointer_iterator<kevent, FDResult>;

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


} // namespace selector
} // namespace

#endif

