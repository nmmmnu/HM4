#include "base.h"



namespace net::worker{



	template<class Protocol, class DBAdapter>
	struct cmd_GET : cmd_base<Protocol, DBAdapter>{
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



} // namespace

