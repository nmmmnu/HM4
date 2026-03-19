#include "base.h"
#include "idgenerator.h"



namespace net::worker::commands::UniqueID{



	template<class Protocol, class DBAdapter>
	struct UNIQUEID : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1 && p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_01);

			auto const id = p.size() == 0 ? uint8_t{0} : from_string<uint8_t>(p[1]);

			using IDGenerator = idgenerator::IDGeneratorTS_HEXMono;

			IDGenerator gen{ id };

			auto buffer = IDGenerator::to_string_buffer_t{};

			return result.set(gen(buffer));
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"uniqueid",		"UNIQUEID",
			"uniqid",		"UNIQID"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "murmur";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				UNIQUEID
			>(pack);
		}
	};



} // namespace


