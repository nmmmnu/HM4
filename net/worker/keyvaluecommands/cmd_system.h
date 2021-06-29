#include "base.h"



namespace net::worker::commands::System{



	template<class Protocol, class DBAdapter>
	struct EXIT : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name = "exit";
		constexpr inline static std::string_view cmd[] = {
			"exit",		"EXIT"
		};

		WorkerStatus operator()(Protocol &, DBAdapter &, IOBuffer &) const final{
			return WorkerStatus::DISCONNECT;
		}
	};

	template<class Protocol, class DBAdapter>
	struct SHUTDOWN : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name = "shutdown";
		constexpr inline static std::string_view cmd[] = {
			"shutdown",	"SHUTDOWN"
		};

		WorkerStatus operator()(Protocol &, DBAdapter &, IOBuffer &) const final{
			return WorkerStatus::SHUTDOWN;
		}
	};



	template<class Protocol, class DBAdapter>
	struct Cointainer{
		EXIT		<Protocol, DBAdapter> exit	;
		SHUTDOWN	<Protocol, DBAdapter> shutdown	;

		template<class Map>
		void registerModule(Map &m){
			registerCmd(m, exit	);
			registerCmd(m, shutdown	);
		}
	};

} // namespace


