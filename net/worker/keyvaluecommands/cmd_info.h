#include "base.h"



namespace net::worker::commands::Info{



	template<class DBAdapter>
	struct INFO : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "info";
		constexpr inline static std::string_view cmd[]	= {
			"info",	"INFO"
		};

		Result process(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) final{
			if (p.size() != 1)
				return Result::error();

			return Result::ok(
				db.info(blob.string)
			);
		}
	};



	template<class DBAdapter>
	struct VERSION : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "version";
		constexpr inline static std::string_view cmd[]	= {
			"version",	"VERSION"
		};

		Result process(ParamContainer const &p, DBAdapter &db, OutputBlob &) final{
			if (p.size() != 1)
				return Result::error();

			return Result::ok(
				db.version()
			);
		}
	};



	template<class DBAdapter>
	struct PING : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "ping";
		constexpr inline static std::string_view cmd[]	= {
			"ping",	"PING"
		};

		Result process(ParamContainer const &p, DBAdapter &, OutputBlob &) final{
			if (p.size() != 1)
				return Result::error();

			return Result::ok("pong");
		}
	};



	template<class DBAdapter>
	struct ECHO : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "echo";
		constexpr inline static std::string_view cmd[]	= {
			"echo",	"ECHO"
		};

		Result process(ParamContainer const &p, DBAdapter &, OutputBlob &blob) final{
			if (p.size() != 2)
				return Result::error();

			// this is input parameter, so need to be saved.
			blob.string = p[1];

			return Result::ok(blob.string);
		}
	};



	template<class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "info";

		static void load(RegisterPack &pack){
			return registerCommands<DBAdapter, RegisterPack,
				INFO	,
				VERSION	,
				PING	,
				ECHO
			>(pack);
		}
	};



} // namespace


