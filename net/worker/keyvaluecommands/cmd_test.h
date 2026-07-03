#include "base.h"
#include "ilist/txguard.h"


namespace net::worker::commands::Test{



	template<class Protocol, class DBAdapter>
	struct TEST : BaseCommandRW<Protocol,DBAdapter>{
		
		TEST() : BaseCommandRW<Protocol,DBAdapter>("TEST", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"test",	"TEST"
		};

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


