#include "base.h"
#include "ilist/txguard.h"


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
			if constexpr(0){
				hm4::TXGuard guard{ *db };

				hm4::insert(*db, "test_a1", "a"	);
				hm4::insert(*db, "test_b1", "b"	);
				hm4::erase (*db, "test_c1"	);

			//	guard.boom_();
			}

			if constexpr(0){
				hm4::TXGuard guard{ *db };

				hm4::insert(*db, "test_a2", "a"	);
				hm4::insert(*db, "test_b2", "b"	);
				hm4::erase (*db, "test_c2"	);

				guard.boom_();
			}

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


