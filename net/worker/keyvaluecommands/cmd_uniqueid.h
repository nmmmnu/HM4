#include "base.h"
#include "idgenerator.h"



namespace net::worker::commands::UniqueID{



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct UNIQUEID : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		UNIQUEID() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("UNIQUEID", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process_(p, result);
		}

	private:
		void process_(ParamContainer const &p, Result<Protocol,MemoryManager> &result){
			if (p.size() != 1 && p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_01);

			auto const id = p.size() == 0 ? uint8_t{0} : from_string<uint8_t>(p[1]);

			using IDGenerator = idgenerator::IDGeneratorTS_HEXMono;

			IDGenerator gen{ id };

			auto buffer = IDGenerator::to_string_buffer_t{};

			return result.set(gen(buffer));
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"uniqueid",		"UNIQUEID",
			"uniqid",		"UNIQID"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter, typename RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "murmur";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, MemoryManager, DBAdapter, RegisterPack,
				UNIQUEID
			>(pack);
		}
	};



} // namespace


