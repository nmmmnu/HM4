#include "base.h"



namespace net::worker::commands::Compat{



	template<class Protocol, class DBAdapter>
	struct SELECT : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "select";
		constexpr inline static std::string_view cmd[]	= {
			"select",	"SELECT"
		};

		constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return result.set();
		}
	};

	template<class Protocol, class DBAdapter>
	struct TYPE : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "type";
		constexpr inline static std::string_view cmd[]	= {
			"type",	"TYPE"
		};

		constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return result.set("string");
		}
	};

	template<class Protocol, class DBAdapter>
	struct TOUCH : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "touch";
		constexpr inline static std::string_view cmd[]	= {
			"touch",	"TOUCH"
		};

		constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return result.set_1();
		}
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "compat";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				SELECT	,
				TYPE	,
				TOUCH
			>(pack);
		}
	};



} // namespace


