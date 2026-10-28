#include "base.h"
#include "ilist/txguard.h"


namespace net::worker::commands::Test{



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct TEST : BaseCommandRW<Protocol,MemoryManager,DBAdapter>{
		
		TEST() : BaseCommandRW<Protocol,MemoryManager,DBAdapter>("TEST", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &, DBAdapter &, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"test",	"TEST"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter, typename RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "test";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, MemoryManager, DBAdapter, RegisterPack,
				TEST
			>(pack);
		}
	};



} // namespace


