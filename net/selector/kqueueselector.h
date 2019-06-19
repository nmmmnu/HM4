#ifndef NET_KQUEUE_SELECTOR_H_
#define NET_KQUEUE_SELECTOR_H_

#include "selectordefs.h"
#include "hidden_pointer_iterator.h"

#include <vector>

struct kevent;

namespace net{
namespace selector{

class KQueueSelector{
	struct HPI{
		using value_type	= kevent;
		using convert_type	= FDResult;

		static bool eq(const value_type *a, const value_type *b)	__attribute__((const));
		static void inc(const value_type * &a);
		static convert_type conv(const value_type *a)			__attribute__((pure));
	};

public:
	constexpr static size_t DEFAULT_SERVER_LIMIT = 64;

	using iterator = hidden_pointer_iterator<HPI>;

	KQueueSelector(size_t server_limit = DEFAULT_SERVER_LIMIT);
	KQueueSelector(KQueueSelector &&other) /* = default */;
	KQueueSelector &operator =(KQueueSelector &&other) /* = default */;
	~KQueueSelector() /* = default */;

	void swap(KQueueSelector &other);

	bool insertFD(int fd, FDEvent event = FDEvent::READ);
	bool updateFD(int fd, FDEvent event);
	bool removeFD(int fd);

	WaitStatus wait(int timeout);

	iterator begin() const;
	iterator end() const;

private:
	int				kqueueFD_;
	std::vector<struct kevent>	fds_;
	int				fdsCount_	= 0;
};


} // namespace selector
} // namespace

#endif

