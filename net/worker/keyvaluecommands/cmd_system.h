#include "base.h"



namespace net::worker::commands::System{



	template<class Protocol, class DBAdapter>
	struct EXIT : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "exit";
		constexpr inline static std::string_view cmd[]	= {
			"exit",		"EXIT",
			"quit",		"QUIT"
		};

		void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return result.set_status(Status::DISCONNECT);
		}
	};

	template<class Protocol, class DBAdapter>
	struct SHUTDOWN : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "shutdown";
		constexpr inline static std::string_view cmd[]	= {
			"shutdown",	"SHUTDOWN"
		};

		void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return result.set_status(Status::SHUTDOWN);
		}
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "system";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				EXIT	,
				SHUTDOWN
			>(pack);
		}
	};



} // namespace


