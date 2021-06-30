#include "base.h"



namespace net::worker::commands::Immutable{



	template<class Protocol, class DBAdapter>
	struct GET : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name = "get";
		constexpr inline static std::string_view cmd[] = {
			"get",	"GET"
		};

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) const final{
			const auto &p = protocol.getParams();

			if (p.size() != 2)
				return error::BadRequest(protocol, buffer);

			const auto &key = p[1];

			if (key.empty())
				return error::BadRequest(protocol, buffer);

			protocol.response_string(buffer, db.get(key));

			return WorkerStatus::WRITE;
		}
	};



	template<class Protocol, class DBAdapter, class Storage, class Map>
	void registerModule(Storage &s, Map &m){
		registerCmd<GET		<Protocol, DBAdapter> >(s, m);
	}



} // namespace

