#include "base.h"

#include "murmur_hash_mixer.h"

namespace net::worker::commands::Random{

	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct RANDOM : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		RANDOM() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("RANDOM", std::begin(cmd__), std::end(cmd__)){}


		// RANDOM / RANDOM 0

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process_(p, result);
		}

	private:
		void process_(ParamContainer const &p, Result<Protocol,MemoryManager> &result){
			if (p.size() != 1 && p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_01);

			auto const index = p.size() == 1 ?
						0 :
						from_string<uint64_t>(p[1])
			;

			uint64_t const r = murmur_hash_mixer64_nz(index);

			return result.set(r);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
				"random",	"RANDOM",	"rand",	"RAND"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct MRANDOM : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		MRANDOM() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("MRANDOM", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			if (p.size() < 2)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_1);

			auto const varg = 1;

			auto &container  = blob.construct<OutputBlob::Container>();
			auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const index = from_string<uint64_t>(*itk);

				uint64_t const r = murmur_hash_mixer64_nz(index);

				bcontainer.push_back();

				container.push_back(
					to_string(r, bcontainer.back())
				);
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
				"mrandom",	"MRANDOM",	"mrand",	"MRAND"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter, typename RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "counters";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, MemoryManager, DBAdapter, RegisterPack,
				RANDOM		,
				MRANDOM
			>(pack);
		}
	};



} // namespace

