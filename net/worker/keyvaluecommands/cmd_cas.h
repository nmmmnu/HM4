#include "base.h"



namespace net::worker::commands::CAS{



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct CAS : BaseCommandRW<Protocol,MemoryManager,DBAdapter>{

		CAS() : BaseCommandRW<Protocol,MemoryManager,DBAdapter>("CAS", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){
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
		constexpr inline static std::string_view cmd__[] = {
			"cas",	"CAS"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct CAD : BaseCommandRW<Protocol,MemoryManager,DBAdapter>{

		CAD() : BaseCommandRW<Protocol,MemoryManager,DBAdapter>("CAD", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){
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
		constexpr inline static std::string_view cmd__[] = {
			"cad",	"CAD"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter, typename RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "cas";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, MemoryManager, DBAdapter, RegisterPack,
				CAS		,
				CAD
			>(pack);
		}
	};


} // namespace


