#include "base.h"



namespace net::worker::commands::System{



	template<class Protocol, class DBAdapter>
	struct EXIT : BaseCommandRO<Protocol,DBAdapter>{
		EXIT() : BaseCommandRO<Protocol,DBAdapter>("EXIT", {
			"exit",		"EXIT",
			"quit",		"QUIT"
		}){}

		void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return result.set_system(Status::DISCONNECT);
		}

	};

	template<class Protocol, class DBAdapter>
	struct SHUTDOWN : BaseCommandRO<Protocol,DBAdapter>{
		SHUTDOWN() : BaseCommandRO<Protocol,DBAdapter>("SHUTDOWN", {
			"shutdown",	"SHUTDOWN"
		}){}

		void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return result.set_system(Status::SHUTDOWN);
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


