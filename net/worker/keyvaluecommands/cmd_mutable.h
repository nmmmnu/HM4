#include "base.h"



namespace net::worker::commands::Mutable{



	template<class Protocol, class DBAdapter>
	struct SET : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "set";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"set",		"SET"
		};

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) const final{
			const auto &p = protocol.getParams();

			if (p.size() != 3 && p.size() != 4)
				return error::BadRequest(protocol, buffer);

			auto const &key = p[1];
			auto const &val = p[2];
			auto const exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

			if (key.empty())
				return error::BadRequest(protocol, buffer);

			db.set(key, val, exp);

			protocol.response_ok(buffer);

			return WorkerStatus::WRITE;
		}
	};



	template<class Protocol, class DBAdapter>
	struct SETEX : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "setex";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"setex",	"SETEX"
		};

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) const final{
			const auto &p = protocol.getParams();

			if (p.size() != 4)
				return error::BadRequest(protocol, buffer);

			auto const &key = p[1];
			auto const &val = p[3];
			auto const exp  = from_string<uint32_t>(p[2]);

			if (key.empty())
				return error::BadRequest(protocol, buffer);

			db.set(key, val, exp);

			protocol.response_ok(buffer);

			return WorkerStatus::WRITE;
		}
	};



	template<class Protocol, class DBAdapter>
	struct SETNX : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "setnx";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"setnx",	"SETNX"
		};

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) const final{
			const auto &p = protocol.getParams();

			if (p.size() != 3 && p.size() != 4)
				return error::BadRequest(protocol, buffer);

			// GET

			const auto &key = p[1];

			if (key.empty())
				return error::BadRequest(protocol, buffer);

			if (! db.get(key).empty()){
				// No Set.

				protocol.response_bool(buffer, false);
			}else{
				// SET

				const auto &val = p[2];
				const auto exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

				db.set(key, val, exp);

				protocol.response_bool(buffer, true);
			}
			// return

			return WorkerStatus::WRITE;
		}
	};



	template<class Protocol, class DBAdapter>
	struct DEL : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "del";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"del",		"DEL"
		};

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) const final{
			const auto &p = protocol.getParams();

			if (p.size() != 2)
				return error::BadRequest(protocol, buffer);

			const auto &key = p[1];

			if (key.empty())
				return error::BadRequest(protocol, buffer);

			protocol.response_bool(buffer, db.del(key));

			return WorkerStatus::WRITE;
		}
	};



	template<class Protocol, class DBAdapter>
	struct GETSET : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "getset";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"getset",	"GETSET"
		};

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) const final{
			const auto &p = protocol.getParams();

			if (p.size() != 3)
				return error::BadRequest(protocol, buffer);

			// GET

			const auto &key = p[1];

			if (key.empty())
				return error::BadRequest(protocol, buffer);

			protocol.response_string(buffer, db.get(key));
			// now old value is inserted in the buffer and
			// we do not care if pair is overwritten

			// SET

			const auto &val = p[2];

			db.set(key, val);

			// return

			return WorkerStatus::WRITE;
		}
	};



	template<class Protocol, class DBAdapter>
	struct EXPIRE : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "expire";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"expire",	"EXPIRE"
		};

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) const final{
			const auto &p = protocol.getParams();

			if (p.size() != 3)
				return error::BadRequest(protocol, buffer);

			// GET

			const auto &key = p[1];

			if (key.empty())
				return error::BadRequest(protocol, buffer);

			auto const &val = db.get(key);

			if (val.empty()){
				protocol.response_bool(buffer, false);
			}else{
				// SET
				auto const exp  = from_string<uint32_t>(p[2]);

				db.set(key, val, exp);

				protocol.response_bool(buffer, true);
			}

			return WorkerStatus::WRITE;
		}
	};



	template<class Protocol, class DBAdapter>
	struct RegisterModule{
		template<class Storage, class Map>
		void operator()(Storage &s, Map &m){
			return registerCommands<Protocol, DBAdapter, Storage, Map,
				SET	,
				SETEX	,
				SETNX	,
				DEL	,
				GETSET	,
				EXPIRE
			>(s, m);
		}
	};


} // namespace


