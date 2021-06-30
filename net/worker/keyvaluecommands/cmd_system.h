#include "base.h"



namespace net::worker::commands::System{



	template<class Protocol, class DBAdapter>
	struct EXIT : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "exit";
		constexpr inline static std::string_view cmd[]	= {
			"exit",		"EXIT"
		};

		WorkerStatus operator()(Protocol &, DBAdapter &, IOBuffer &) const final{
			return WorkerStatus::DISCONNECT;
		}
	};

	template<class Protocol, class DBAdapter>
	struct SHUTDOWN : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "shutdown";
		constexpr inline static std::string_view cmd[]	= {
			"shutdown",	"SHUTDOWN"
		};

		WorkerStatus operator()(Protocol &, DBAdapter &, IOBuffer &) const final{
			return WorkerStatus::SHUTDOWN;
		}
	};



	template<class Protocol, class DBAdapter, class Storage, class Map>
	void registerModule(Storage &s, Map &m){
		registerCmd<EXIT	<Protocol, DBAdapter> >(s, m);
		registerCmd<SHUTDOWN	<Protocol, DBAdapter> >(s, m);
	}



} // namespace


