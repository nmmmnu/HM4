#include "base.h"
#include "mystring.h"



namespace net::worker::commands::Immutable{



	template<class Protocol, class DBAdapter>
	struct GET : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "get";
		constexpr inline static std::string_view cmd[]	= {
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



	template<class Protocol, class DBAdapter>
	struct TTL : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "ttl";
		constexpr inline static std::string_view cmd[]	= {
			"ttl",	"TTL"
		};

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) const final{
			const auto &p = protocol.getParams();

			if (p.size() != 2)
				return error::BadRequest(protocol, buffer);

			const auto &key = p[1];

			if (key.empty())
				return error::BadRequest(protocol, buffer);

			to_string_buffer_t std_buffer;

			std::string_view const val = to_string(db.ttl(key), std_buffer);

			protocol.response_string(buffer, val);

			return WorkerStatus::WRITE;
		}
	};



	template<class Protocol, class DBAdapter>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "immutable";

		template<class Storage, class Map>
		static void load(Storage &s, Map &m){
			return registerCommands<Protocol, DBAdapter, Storage, Map,
				GET	,
				TTL
			>(s, m);
		}
	};



} // namespace

