#include "base.h"
#include "murmur_hash_64a.h"



namespace net::worker::commands::Murmur{



	template<class Protocol, class DBAdapter>
	struct MURMUR : BaseCommandRO<Protocol,DBAdapter>{

		MURMUR() : BaseCommandRO<Protocol,DBAdapter>("MURMUR", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return process_(p, result);
		}

	private:
		void process_(ParamContainer const &p, Result<Protocol> &result){
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



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "murmur";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				MURMUR
			>(pack);
		}
	};



} // namespace


