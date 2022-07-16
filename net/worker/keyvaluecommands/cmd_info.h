#include "base.h"



namespace net::worker::commands::Info{



	template<class Protocol, class DBAdapter>
	struct INFO : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "info";
		constexpr inline static std::string_view cmd[]	= {
			"info",	"INFO"
		};

		Result operator()(Protocol &protocol, ParamContainer const &p, DBAdapter &db, IOBuffer &buffer) const final{
			if (p.size() != 1)
				return Status::ERROR;

			protocol.response_string(buffer, db.info());

			return {};
		}
	};



	template<class Protocol, class DBAdapter>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "info";

		template<class Storage, class Map>
		static void load(Storage &s, Map &m){
			return registerCommands<Protocol, DBAdapter, Storage, Map,
				INFO
			>(s, m);
		}
	};



} // namespace


