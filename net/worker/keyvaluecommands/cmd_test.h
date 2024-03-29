#include "base.h"



namespace net::worker::commands::Test{



	template<class Protocol, class DBAdapter>
	struct TEST : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			[[maybe_unused]]
			auto const &list = db->mutable_list();

		//	list.testIntegrity(std::false_type{});
		//	list.testIntegrity(std::true_type{});

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
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


