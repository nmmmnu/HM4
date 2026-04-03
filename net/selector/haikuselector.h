#ifndef NET_POLL_SELECTOR_H_
#define NET_POLL_SELECTOR_H_

#include "selectordefs.h"
#include "hidden_pointer_iterator.h"

#include <vector>
#include <string_view>

struct object_wait_info;

namespace net{
namespace selector{

class HaikuSelector{
	struct HPI{
		using value_type	= object_wait_info;
		using convert_type	= FDResult;

		static bool eq(const value_type *a, const value_type *b){
			return a == b;
		}

		static void inc(const value_type * &a);
		static convert_type conv(const value_type *a)	__attribute__((pure));
	};

public:
	using iterator = hidden_pointer_iterator<HPI>;

	static inline constexpr std::string_view NAME = "wait_for_objects";

	HaikuSelector(uint32_t maxFD);
	HaikuSelector(HaikuSelector &&other) /* = default */;
	HaikuSelector &operator =(HaikuSelector &&other) /* = default */;
	~HaikuSelector() /* = default */;

	bool insertFD(int fd, FDEvent event = FDEvent::READ);
	bool updateFD(int fd, FDEvent event);
	bool removeFD(int fd);

	WaitStatus wait(int timeout);

	iterator begin() const;
	iterator end() const;

private:
	std::vector<struct object_wait_info>	fds_;
};



} // namespace selector
} // namespace

#endif

