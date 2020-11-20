#include "base.h"



namespace net::worker{



	template<class Protocol, class DBAdapter>
	struct cmd_SET : cmd_base<Protocol, DBAdapter>{
		constexpr inline static std::string_view cmd[] = {
			"set",	"SET"
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
	struct cmd_SETEX : cmd_base<Protocol, DBAdapter>{
		constexpr inline static std::string_view cmd[] = {
			"setex",
			"SETEX"
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
	struct cmd_DEL : cmd_base<Protocol, DBAdapter>{
		constexpr inline static std::string_view cmd[] = {
			"del",
			"DEL"
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
	struct cmd_GETSET : cmd_base<Protocol, DBAdapter>{
		constexpr inline static std::string_view cmd[] = {
			"getset",
			"GETSET"
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



} // namespace


