#include "base.h"



namespace net::worker::commands_reload{



	template<class Protocol, class DBAdapter>
	struct cmd_SAVE : cmd_base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name = "save";
		constexpr inline static std::string_view cmd[] = {
			"save",		"SAVE",
			"bgsave",	"BGSAVE"
		};

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) const final{
			const auto &p = protocol.getParams();

			if (p.size() != 1)
				return error::BadRequest(protocol, buffer);

			db.save();

			protocol.response_ok(buffer);

			return WorkerStatus::WRITE;
		}
	};

	template<class Protocol, class DBAdapter>
	struct cmd_RELOAD : cmd_base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name = "reload";
		constexpr inline static std::string_view cmd[] = {
			"reload",
			"RELOAD"
		};

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) const final{
			const auto &p = protocol.getParams();

			if (p.size() != 1)
				return error::BadRequest(protocol, buffer);

			db.reload();

			protocol.response_ok(buffer);

			return WorkerStatus::WRITE;
		}
	};



} // namespace


