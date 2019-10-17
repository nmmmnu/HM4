#ifndef NET_POLL_SELECTOR_H_
#define NET_POLL_SELECTOR_H_

#include "selectordefs.h"
#include "hidden_pointer_iterator.h"

#include <vector>

struct pollfd;

namespace net{
namespace selector{

class PollSelector{
	struct HPI{
		using value_type	= pollfd;
		using convert_type	= FDResult;

		static bool eq(const value_type *a, const value_type *b)	__attribute__((const));
		static void inc(const value_type * &a);
		static convert_type conv(const value_type *a)			__attribute__((pure));
	};

public:
	using iterator = hidden_pointer_iterator<HPI>;

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
	std::vector<struct pollfd>	fds_;
};



} // namespace selector
} // namespace

#endif

