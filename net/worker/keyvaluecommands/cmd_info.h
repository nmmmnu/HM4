#include "base.h"



namespace net::worker{



	template<class Protocol, class DBAdapter>
	struct cmd_INFO : cmd_base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name = "info";
		constexpr inline static std::string_view cmd[] = {
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



} // namespace


