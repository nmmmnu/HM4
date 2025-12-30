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



		template<typename ParamContainer, typename OutputBlob, typename Result, typename DBAdapter>
		void cmdProcessExists(ParamContainer const &p, DBAdapter &db, Result &result, OutputBlob &){
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

	} // namespace impl_



	template<class Protocol, class DBAdapter>
	struct EXISTS : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// XXXEXISTS key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace impl_;

			return cmdProcessExists(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
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

			"ixmexists"	,	"IXMEXISTS"	,
			"ixtexists"	,	"IXTEXISTS"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "exists_shared";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				EXISTS
			>(pack);
		}
	};

} // namespace net::worker::commands::MIndexShared

