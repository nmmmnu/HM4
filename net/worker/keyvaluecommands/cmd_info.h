#include "base.h"



namespace net::worker::commands::Info{



	template<class Protocol, class DBAdapter>
	struct INFO : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "info";
		constexpr inline static std::string_view cmd[]	= {
			"info",	"INFO"
		};

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) const final{
			const auto &p = protocol.getParams();

			if (p.size() != 1)
				return error::BadRequest(protocol, buffer);

			protocol.response_string(buffer, db.info());

			return WorkerStatus::WRITE;
		}
	};



	template<class Protocol, class DBAdapter>
	struct RegisterModule{
		template<class Storage, class Map>
		static void go(Storage &s, Map &m){
			return registerCommands<Protocol, DBAdapter, Storage, Map,
				INFO
			>(s, m);
		}
	};



} // namespace


