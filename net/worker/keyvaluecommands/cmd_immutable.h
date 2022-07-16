#include "base.h"
#include "mystring.h"



namespace net::worker::commands::Immutable{



	template<class Protocol, class DBAdapter>
	struct GET : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "get";
		constexpr inline static std::string_view cmd[]	= {
			"get",	"GET"
		};

		Result operator()(Protocol &protocol, ParamContainer const &p, DBAdapter &db, IOBuffer &buffer) const final{
			if (p.size() != 2)
				return Status::ERROR;

			const auto &key = p[1];

			if (key.empty())
				return Status::ERROR;

			protocol.response_string(buffer, db.get(key));

			return {};
		}
	};



	template<class Protocol, class DBAdapter>
	struct TTL : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "ttl";
		constexpr inline static std::string_view cmd[]	= {
			"ttl",	"TTL"
		};

		Result operator()(Protocol &protocol, ParamContainer const &p, DBAdapter &db, IOBuffer &buffer) const final{
			if (p.size() != 2)
				return Status::ERROR;

			const auto &key = p[1];

			if (key.empty())
				return Status::ERROR;

			to_string_buffer_t std_buffer;

			std::string_view const val = to_string(db.ttl(key), std_buffer);

			protocol.response_string(buffer, val);

			return {};
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

