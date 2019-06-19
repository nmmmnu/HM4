#ifndef NET_POLL_SELECTOR_H_
#define NET_POLL_SELECTOR_H_

#include "selectordefs.h"

#include <vector>

struct pollfd;

namespace hpi{
	using hidden_t = pollfd;
	using net::selector::FDResult;

	bool eq(const hidden_t *a, const hidden_t *b)	__attribute__((const))	;
	void inc(const hidden_t * &a)						;
	FDResult conv(const hidden_t *a)		__attribute__((const))	;
}

#include "hidden_pointer_iterator.h"

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

