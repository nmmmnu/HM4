#include "base.h"



namespace net::worker{



	template<class Protocol, class DBAdapter>
	struct cmd_EXIT : cmd_base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name = "exit";
		constexpr inline static std::string_view cmd[] = {
			"exit",		"EXIT"
		};

		WorkerStatus operator()(Protocol &, DBAdapter &, IOBuffer &) const final{
			return WorkerStatus::DISCONNECT;
		}
	};

	template<class Protocol, class DBAdapter>
	struct cmd_SHUTDOWN : cmd_base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name = "shutdown";
		constexpr inline static std::string_view cmd[] = {
			"shutdown",	"SHUTDOWN"
		};

		WorkerStatus operator()(Protocol &, DBAdapter &, IOBuffer &) const final{
			return WorkerStatus::SHUTDOWN;
		}
	};


} // namespace


