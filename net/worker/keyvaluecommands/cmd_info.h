#include "base.h"



namespace net::worker::commands::Info{



	template<class DBAdapter>
	struct INFO : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "info";
		constexpr inline static std::string_view cmd[]	= {
			"info",	"INFO"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) const final{
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

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) const final{
			if (p.size() != 1)
				return Result::error();

			return Result::ok(
				db.version()
			);
		}
	};



	template<class DBAdapter>
	struct TYPE : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "type";
		constexpr inline static std::string_view cmd[]	= {
			"type",	"TYPE"
		};

		Result operator()(ParamContainer const &p, DBAdapter &, OutputBlob &) const final{
			if (p.size() != 2)
				return Result::error();

			return Result::ok(
				"string"
			);
		}
	};



	template<class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "info";

		static void load(RegisterPack &pack){
			return registerCommands<DBAdapter, RegisterPack,
				INFO	,
				VERSION	,
				TYPE
			>(pack);
		}
	};



} // namespace


