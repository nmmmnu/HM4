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



	template<class Protocol, class DBAdapter, class Storage, class Map>
	void registerModule(Storage &s, Map &m){
		registerCmd<INFO	, Protocol, DBAdapter>(s, m);
	}



} // namespace


