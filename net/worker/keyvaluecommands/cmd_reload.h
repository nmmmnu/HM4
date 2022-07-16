#include "base.h"



namespace net::worker::commands::Reload{



	template<class Protocol, class DBAdapter>
	struct SAVE : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "save";
		constexpr inline static std::string_view cmd[]	= {
			"save",		"SAVE",
			"bgsave",	"BGSAVE"
		};

		Result operator()(Protocol &protocol, ParamContainer const &p, DBAdapter &db, IOBuffer &buffer) const final{
			if (p.size() != 1)
				return Status::ERROR;

			db.save();

			protocol.response_ok(buffer);

			return {};
		}
	};

	template<class Protocol, class DBAdapter>
	struct RELOAD : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "reload";
		constexpr inline static std::string_view cmd[]	= {
			"reload",	"RELOAD"
		};

		Result operator()(Protocol &protocol, ParamContainer const &p, DBAdapter &db, IOBuffer &buffer) const final{
			if (p.size() != 1)
				return Status::ERROR;

			db.reload();

			protocol.response_ok(buffer);

			return {};
		}
	};



	template<class Protocol, class DBAdapter>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "reload";

		template<class Storage, class Map>
		static void load(Storage &s, Map &m){
			return registerCommands<Protocol, DBAdapter, Storage, Map,
				SAVE	,
				RELOAD
			>(s, m);
		}
	};



} // namespace


