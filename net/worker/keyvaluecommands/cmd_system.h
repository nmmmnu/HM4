#include "base.h"



namespace net::worker::commands::System{



	template<class DBAdapter>
	struct EXIT : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "exit";
		constexpr inline static std::string_view cmd[]	= {
			"exit",		"EXIT",
			"quit",		"QUIT"
		};

		Result operator()(ParamContainer const &, DBAdapter &, OutputBlob &) const final{
			return { Status::DISCONNECT, nullptr };
		}
	};

	template<class DBAdapter>
	struct SHUTDOWN : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "shutdown";
		constexpr inline static std::string_view cmd[]	= {
			"shutdown",	"SHUTDOWN"
		};

		Result operator()(ParamContainer const &, DBAdapter &, OutputBlob &) const final{
			return { Status::SHUTDOWN, nullptr };
		}
	};



	template<class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "system";

		static void load(RegisterPack &pack){
			return registerCommands<DBAdapter, RegisterPack,
				EXIT	,
				SHUTDOWN
			>(pack);
		}
	};



} // namespace


