#ifndef NET_POLL_SELECTOR_H_
#define NET_POLL_SELECTOR_H_

#include "selectordefs.h"
#include "hidden_pointer_iterator.h"
#include "sparsearray/sparsearray.h"

#include <string_view>

struct pollfd;

namespace net{
namespace selector{

class PollSelector{
	struct HPI{
		using value_type	= pollfd;
		using convert_type	= FDResult;

		static bool eq(const value_type *a, const value_type *b){
			return a == b;
		}

		static void inc(const value_type * &a);
		static convert_type conv(const value_type *a)	__attribute__((pure));
	};

	struct SparseMapController{
		using key_type		= uint32_t;
		using value_type	= pollfd;
		using mapped_type	= pollfd;

		[[nodiscard]]
		static key_type getKey(mapped_type const &value);

		[[nodiscard]]
		static value_type const &getVal(mapped_type const &value);

		[[nodiscard]]
		static value_type       &getVal(mapped_type       &value);
	};

public:
	using iterator = hidden_pointer_iterator<HPI>;

	static inline constexpr std::string_view NAME = "poll";

	PollSelector(uint32_t const conf_rlimitNoFile, uint32_t const conf_max_clients);
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
	mysparsearray::SparseArray<uint32_t, SparseMapController> fds_;
};



} // namespace selector
} // namespace

#endif

