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

		static bool eq(const value_type *a, const value_type *b);
		static void inc(const value_type * &a);
		static convert_type conv(const value_type *a);
	};

public:
	constexpr static size_t DEFAULT_SERVER_LIMIT = 64;

	using iterator = hidden_pointer_iterator<HPI>;

	EPollSelector(size_t server_limit = DEFAULT_SERVER_LIMIT);
	EPollSelector(EPollSelector &&other);
	EPollSelector &operator =(EPollSelector &&other);
	~EPollSelector();

	void swap(EPollSelector &other);

	bool insertFD(int fd, FDEvent event = FDEvent::READ);
	bool updateFD(int fd, FDEvent event);
	bool removeFD(int fd);

	WaitStatus wait(int timeout);

	iterator begin() const;
	iterator end() const;

private:
	int					epollFD_;
	std::vector<epoll_event>		fds_;
	int					fdsCount_	= 0;
};


} // namespace selector
} // namespace

#endif

