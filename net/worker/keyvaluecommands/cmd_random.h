#include "base.h"

#include "murmur_hash_mixer.h"

namespace net::worker::commands::Random{

	template<class Protocol, class DBAdapter>
	struct RANDOM : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// RANDOM / RANDOM 0

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1 && p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_01);

			auto const index = p.size() == 1 ?
						0 :
						from_string<uint64_t>(p[1])
			;

			uint64_t const r = murmur_hash_mixer64(index + 1);

			return result.set(r);
		}

	private:
		constexpr inline static std::string_view cmd[] = {
				"random",	"RANDOM",	"rand",	"RAND"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MRANDOM : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 2)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_1);

			auto &container  = blob.container();
			auto &bcontainer = blob.bcontainer();

			auto const varg = 1;

			if (container.capacity() < p.size() - varg)
				return result.set_error(ResultErrorMessages::CONTAINER_CAPACITY);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const index = from_string<uint64_t>(*itk);

				uint64_t const r = murmur_hash_mixer64(index + 1);

				bcontainer.push_back();

				container.push_back(
					to_string(r, bcontainer.back())
				);
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[] = {
				"mrandom",	"MRANDOM",	"mrand",	"MRAND"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "counters";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				RANDOM		,
				MRANDOM
			>(pack);
		}
	};



} // namespace

