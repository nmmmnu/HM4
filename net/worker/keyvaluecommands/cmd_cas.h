#include "base.h"



namespace net::worker::commands::CAS{



	template<class Protocol, class DBAdapter>
	struct CAS : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 4 && p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_34);


			// GET

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto &old_val = p[2];

			if (!hm4::Pair::isValValidNZ(old_val))
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			const auto &val = p[3];

			if (!hm4::Pair::isValValidNZ(val))
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			if (auto *it = hm4::getPairPtr(*db, key); it && it->getVal() == old_val){
				// SET

				auto const exp  = p.size() == 5 ? from_string<uint32_t>(p[4]) : 0;

				// HINT
				const auto *hint = & *it;
				hm4::insertHintF<hm4::PairFactory::Normal>(*db, hint, key, val, exp);

				return result.set(true);
			}

			return result.set(false);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cas",	"CAS"
		};
	};



	template<class Protocol, class DBAdapter>
	struct CAD : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			// GET

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto &old_val = p[2];

			if (!hm4::Pair::isValValidNZ(old_val))
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			if (auto *it = hm4::getPairPtr(*db, key); it && it->getVal() == old_val){
				// DEL

				// HINT
				const auto *hint = & *it;
				// put tombstone
				hm4::insertHintF<hm4::PairFactory::Tombstone>(*db, hint, key);

				return result.set(true);
			}

			return result.set(false);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cad",	"CAD"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "cas";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				CAS		,
				CAD
			>(pack);
		}
	};


} // namespace


