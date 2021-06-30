#include "base.h"



namespace net::worker{



	template<class Protocol, class DBAdapter>
	struct XXX : cmd_base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name = "xxx";
		constexpr inline static std::string_view cmd[] = {
			"xxx",	"XXX"
		};

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) const final{
		}
	};



	template<class Protocol, class DBAdapter, class Storage, class Map>
	void registerModule(Storage &s, Map &m){
		registerCmd<XXX		<Protocol, DBAdapter> >(s, m);
	}



} // namespace

