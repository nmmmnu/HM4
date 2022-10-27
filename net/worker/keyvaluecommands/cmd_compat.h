#include "base.h"



namespace net::worker::commands::Compat{



	template<class DBAdapter>
	struct SELECT : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "select";
		constexpr inline static std::string_view cmd[]	= {
			"select",	"SELECT"
		};

		constexpr Result operator()(ParamContainer const &, DBAdapter &, OutputBlob &) const final{
			return Result::ok();
		}
	};

	template<class DBAdapter>
	struct TYPE : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "type";
		constexpr inline static std::string_view cmd[]	= {
			"type",	"TYPE"
		};

		constexpr Result operator()(ParamContainer const &, DBAdapter &, OutputBlob &) const final{
			return Result::ok(
				"string"
			);
		}
	};

	template<class DBAdapter>
	struct TOUCH : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "touch";
		constexpr inline static std::string_view cmd[]	= {
			"touch",	"TOUCH"
		};

		constexpr Result operator()(ParamContainer const &, DBAdapter &, OutputBlob &) const final{
			return Result::ok(
				1
			);
		}
	};



	template<class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "compat";

		static void load(RegisterPack &pack){
			return registerCommands<DBAdapter, RegisterPack,
				SELECT	,
				TYPE	,
				TOUCH
			>(pack);
		}
	};



} // namespace


