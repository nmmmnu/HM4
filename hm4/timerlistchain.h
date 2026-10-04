#ifndef TIMER_LIST_CHAIN_H_
#define TIMER_LIST_CHAIN_H_

namespace hm4::chain{
	namespace impl_{
		char const ANY		= 'A';
		char const READ		= 'R';
		char const WRITE	= 'W';
	}

	template<uint64_t ID>
	struct log_histogram{
		char		mode	= ANY;

		constexpr static char ANY	= impl_::ANY	;
		constexpr static char READ	= impl_::READ	;
		constexpr static char WRITE	= impl_::WRITE	;
	};

	template<uint64_t ID>
	struct reset_histogram{
		char		mode	= ANY;

		constexpr static char ANY	= impl_::ANY	;
		constexpr static char READ	= impl_::READ	;
		constexpr static char WRITE	= impl_::WRITE	;
	};

	template<uint64_t ID>
	struct get_histogram{
		char		mode	= ANY;

		constexpr static char ANY	= impl_::ANY	;
		constexpr static char READ	= impl_::READ	;
		constexpr static char WRITE	= impl_::WRITE	;
	};
}

#endif

