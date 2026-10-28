#include "base.h"
#include "murmur_hash_64a.h"



namespace net::worker::commands::Murmur{



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct MURMUR : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		MURMUR() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("MURMUR", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process_(p, result);
		}

	private:
		void process_(ParamContainer const &p, Result<Protocol,MemoryManager> &result){
			if (p.size() < 2 || p.size() > 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_234);

			auto const &val = p[1];

			auto const seed = p.size() >= 3 ? from_string<uint64_t>(p[2]) : 0;

			auto const mod  = p.size() == 4 ? from_string<uint64_t>(p[3]) : 0;

			auto const hash = murmur_hash64a(val, seed);

			if (mod)
				return result.set(hash % mod);
			else
				return result.set(hash);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"murmur",		"MURMUR",
			"murmurhash64a",	"MURMURHASH64A"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter, typename RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "murmur";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, MemoryManager, DBAdapter, RegisterPack,
				MURMUR
			>(pack);
		}
	};



} // namespace


