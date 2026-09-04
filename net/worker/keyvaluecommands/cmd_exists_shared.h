#include "base.h"

#include "pair.h"

namespace net::worker::commands::ExistsShared{
	namespace impl_{
		constexpr static bool valid(std::string_view keyN, std::string_view keySub, size_t more = 0){
			// keyN~word~sort~keySub, 3 * ~
			return hm4::Pair::isCompositeKeyValid(3 + more, keyN, keySub);
		}

		std::string_view makeKeyCtrl(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub){

			return concatenateBuffer(bufferKey,
					keyN		,	separator	,
								separator	,
					keySub
			);
		}

		template<typename DBAdapter>
		bool exists(DBAdapter &db,
				std::string_view keyN, std::string_view keySub){

			hm4::PairBufferKey bufferKeyCtrl;
			auto const keyCtrl = makeKeyCtrl(bufferKeyCtrl, DBAdapter::SEPARATOR, keyN, keySub);

			logger<Logger::DEBUG>() << "ZSetMulti::EXISTS: ctrl key" << keyCtrl;

			return hm4::getPairOK(*db, keyCtrl);
		}



		template<typename ParamContainer, typename Result, typename DBAdapter>
		void cmdProcessExists(ParamContainer const &p, DBAdapter &db, Result &result){
			// EXISTS key subkey0

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN   = p[1];
			auto const &keySub = p[2];

			if (keyN.empty() || keySub.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!valid(keyN, keySub))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			return result.set(
				exists(db, keyN, keySub)
			);
		}

		template<typename ParamContainer, typename OutputBlob, typename Result, typename DBAdapter>
		void cmdProcessMExists(ParamContainer const &p, DBAdapter &db, Result &result, OutputBlob &blob){
			// MEXISTS key subkey0 subkey1...

			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_3);

			const auto &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (auto const &keySub = *itk; !valid(keyN, keySub))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto &container = blob. template construct<typename OutputBlob::Container>();

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				if (auto const &keySub = *itk; exists(db, keyN, keySub))
					container.push_back("1");
				else
					container.push_back("0");
			}

			return result.set_container(container);
		}

	} // namespace impl_



	template<class Protocol, class DBAdapter>
	struct EXISTS : BaseCommandRO<Protocol,DBAdapter>{

		EXISTS() : BaseCommandRO<Protocol,DBAdapter>("EXISTS", std::begin(cmd__), std::end(cmd__)){}

		// XXXEXISTS key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace impl_;

			return cmdProcessExists(p, db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"mc1exists"	,	"MC1EXISTS"	,
			"mc2exists"	,	"MC2EXISTS"	,
			"mc3exists"	,	"MC3EXISTS"	,
			"mc4exists"	,	"MC4EXISTS"	,
			"mc8exists"	,	"MC8EXISTS"	,
			"mc16exists"	,	"MC16EXISTS"	,

			"ix1exists"	,	"IX1EXISTS"	,
			"ix2exists"	,	"IX2EXISTS"	,
			"ix3exists"	,	"IX3EXISTS"	,
			"ix4exists"	,	"IX4EXISTS"	,
			"ix5exists"	,	"IX5EXISTS"	,
			"ix6exists"	,	"IX6EXISTS"	,

			"mhexists"	,	"MHEXISTS"	,
			"vexists"	,	"VEXISTS"	,

			"ixmexists"	,	"IXMEXISTS"
		};
	};

	template<class Protocol, class DBAdapter>
	struct MEXISTS : BaseCommandRO<Protocol,DBAdapter>{

		MEXISTS() : BaseCommandRO<Protocol,DBAdapter>("MEXISTS", std::begin(cmd__), std::end(cmd__)){}

		// XXXEXISTS key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace impl_;

			return cmdProcessMExists(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ix1mexists"	,	"IX1MEXISTS"	,
			"ix2mexists"	,	"IX2MEXISTS"	,
			"ix3mexists"	,	"IX3MEXISTS"	,
			"ix4mexists"	,	"IX4MEXISTS"	,
			"ix5mexists"	,	"IX5MEXISTS"	,
			"ix6mexists"	,	"IX6MEXISTS"	,

			"mhmexists"	,	"MHMEXISTS"	,
			"vmexists"	,	"VMEXISTS"	,

			"ixmmexists"	,	"IXMMEXISTS"
		};

	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "exists_shared";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				EXISTS	,
				MEXISTS
			>(pack);
		}
	};

} // namespace net::worker::commands::MIndexShared

