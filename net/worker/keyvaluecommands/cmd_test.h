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
			auto const &list = db->mutable_list();

			size_t size = 0;
			for(auto it = std::begin(list); it != std::end(list); ++it){
				logger<Logger::DEBUG>() << "TEST:" << it->getKey();

				++size;
				if (size > list.size()){
					logger<Logger::DEBUG>() << "TEST: something is wrong";
					for(size_t i = 0; i < 100; ++i){
						logger<Logger::DEBUG>() << "TEST:" << it->getKey();
						++it;
					}

					break;
				}
			}

			logger<Logger::DEBUG>() << "TEST: OK, SIZE: " << size;

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"test",	"TEST"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "info";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				TEST
			>(pack);
		}
	};



} // namespace


