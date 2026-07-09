#include "base.h"



namespace net::worker::commands::System{



	template<class Protocol, class DBAdapter>
	struct EXIT : BaseCommandRO<Protocol,DBAdapter>{

		EXIT() : BaseCommandRO<Protocol,DBAdapter>("EXIT", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return result.set_system(Status::DISCONNECT);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"exit",		"EXIT",
			"quit",		"QUIT"
		};

	};

	template<class Protocol, class DBAdapter>
	struct SHUTDOWN : BaseCommandRO<Protocol,DBAdapter>{

		SHUTDOWN() : BaseCommandRO<Protocol,DBAdapter>("SHUTDOWN", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return result.set_system(Status::SHUTDOWN);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"shutdown",	"SHUTDOWN"
		};

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


