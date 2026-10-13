#ifndef TIMER_LIST_CHAIN_H_
#define TIMER_LIST_CHAIN_H_

namespace hm4::chain{

	template<uint64_t ID>
	struct get_histogram{
		char		mode;

		constexpr static char READ	= 'R';
		constexpr static char WRITE	= 'W';
	};
}

#endif

