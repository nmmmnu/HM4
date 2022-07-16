#include "base.h"



namespace net::worker::commands::Info{



	template<class DBAdapter>
	struct INFO : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "info";
		constexpr inline static std::string_view cmd[]	= {
			"info",	"INFO"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputContainer &) const final{
			if (p.size() != 1)
				return Result::error();

			// db.info() "probably" return a std::string.

			return Result::ok(std::move(db.info()));
		}
	};



	template<class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "info";

		static void load(RegisterPack &pack){
			return registerCommands<DBAdapter, RegisterPack,
				INFO
			>(pack);
		}
	};



} // namespace


