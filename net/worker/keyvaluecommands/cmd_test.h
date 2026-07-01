#include "base.h"
#include "ilist/txguard.h"


namespace net::worker::commands::Test{



	template<class Protocol, class DBAdapter>
	struct TEST : BaseCommandRW<Protocol,DBAdapter>{
		TEST() : BaseCommandRW<Protocol,DBAdapter>("TEST", {
			"test",	"TEST"
		}){}

		void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return result.set();
		}

	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "test";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				TEST
			>(pack);
		}
	};



} // namespace


