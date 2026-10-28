#include "base.h"



namespace net::worker::commands::System{



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct EXIT : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		EXIT() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("EXIT", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &, DBAdapter &, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return result.set_system(Status::DISCONNECT);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"exit",		"EXIT",
			"quit",		"QUIT"
		};

	};

	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct SHUTDOWN : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		SHUTDOWN() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("SHUTDOWN", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &, DBAdapter &, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return result.set_system(Status::SHUTDOWN);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"shutdown",	"SHUTDOWN"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter, typename RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "system";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, MemoryManager, DBAdapter, RegisterPack,
				EXIT	,
				SHUTDOWN
			>(pack);
		}
	};



} // namespace


