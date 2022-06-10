#include "base.h"



namespace net::worker::commands::Reload{



	template<class Protocol, class DBAdapter>
	struct SAVE : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "save";
		constexpr inline static std::string_view cmd[]	= {
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
	struct RELOAD : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "reload";
		constexpr inline static std::string_view cmd[]	= {
			"reload",	"RELOAD"
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



	template<class Protocol, class DBAdapter, class Storage, class Map>
	void registerModule(Storage &s, Map &m){
		return registerCommands<Protocol, DBAdapter, Storage, Map,
			SAVE	,
			RELOAD
		>(s, m);
	}



} // namespace


