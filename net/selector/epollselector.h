#ifndef NET_EPOLL_SELECTOR_H_
#define NET_EPOLL_SELECTOR_H_

#include "selectordefs.h"
#include "hidden_pointer_iterator.h"

#include <vector>

struct epoll_event;

namespace net{
namespace selector{

class EPollSelector{
	struct HPI{
		using value_type	= epoll_event;
		using convert_type	= FDResult;

		static bool eq(const value_type *a, const value_type *b)	__attribute__((const));
		static void inc(const value_type * &a);
		static convert_type conv(const value_type *a)			__attribute__((const));
	};

public:
	using iterator = hidden_pointer_iterator<HPI>;

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


} // namespace selector
} // namespace

#endif

