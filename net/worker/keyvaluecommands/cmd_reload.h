#include "base.h"



namespace net::worker::commands::Reload{



	template<class DBAdapter>
	struct SAVE : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "save";
		constexpr inline static std::string_view cmd[]	= {
			"save",		"SAVE",
			"bgsave",	"BGSAVE"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) final{
			if (p.size() != 1)
				return Result::error();

			db.save();

			return Result::ok();
		}
	};

	template<class DBAdapter>
	struct RELOAD : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "reload";
		constexpr inline static std::string_view cmd[]	= {
			"reload",	"RELOAD"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) final{
			if (p.size() != 1)
				return Result::error();

			db.reload();

			return Result::ok();
		}
	};



	template<class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "reload";

		static void load(RegisterPack &pack){
			return registerCommands<DBAdapter, RegisterPack,
				SAVE	,
				RELOAD
			>(pack);
		}
	};



} // namespace


