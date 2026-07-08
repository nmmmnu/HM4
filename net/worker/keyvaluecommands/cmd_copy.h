#include "base.h"
#include "ilist/txguard.h"


namespace net::worker::commands::Copy{
	namespace impl_{

		enum class CPMVOperation{
			MV	,
			MV_NX	,
			CP	,
			CP_NX
		};

		template<CPMVOperation operation, class Protocol, class List>
		void cpmv(ParamContainer const &p, List &list, Result<Protocol> &result){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			// GET

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto &newkey = p[2];

			if (!hm4::Pair::isKeyValid(newkey))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (key == newkey)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			if constexpr(operation == CPMVOperation::CP_NX || operation == CPMVOperation::MV_NX){

				if ( hm4::getPairOK(list, newkey) )
					return result.set(false);
			}

			if (auto *it = hm4::getPairPtr(list, key); it){

				if constexpr(operation == CPMVOperation::MV || operation == CPMVOperation::MV_NX){
					// Move  operation

					[[maybe_unused]]
					hm4::TXGuard guard{ list };

					hm4::insert(list, newkey, it->getVal(), it->getTTL());
					hm4::erase(list, key);
				}else{
					// Copy operation

					hm4::insert(list, newkey, it->getVal(), it->getTTL());
				}

				return result.set(true);
			}else{
				return result.set(false);
			}
		}

	} // namespace impl_



	template<class Protocol, class DBAdapter>
	struct RENAME : BaseCommandRW<Protocol,DBAdapter>{

		RENAME() : BaseCommandRW<Protocol,DBAdapter>("RENAME", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace impl_;
			return cpmv<CPMVOperation::MV>(p, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"rename",	"RENAME"	,
			"move",		"MOVE"		,
		};

	};



	template<class Protocol, class DBAdapter>
	struct RENAMENX : BaseCommandRW<Protocol,DBAdapter>{

		RENAMENX() : BaseCommandRW<Protocol,DBAdapter>("RENAMENX", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace impl_;
			return cpmv<CPMVOperation::MV_NX>(p, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"renamenx",	"RENAMENX"	,
			"movenx",	"MOVENX"
		};

	};



	template<class Protocol, class DBAdapter>
	struct COPY : BaseCommandRW<Protocol,DBAdapter>{

		COPY() : BaseCommandRW<Protocol,DBAdapter>("COPY", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace impl_;
			return cpmv<CPMVOperation::CP>(p, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"copy",	"COPY"
		};

	};



	template<class Protocol, class DBAdapter>
	struct COPYNX : BaseCommandRW<Protocol,DBAdapter>{

		COPYNX() : BaseCommandRW<Protocol,DBAdapter>("COPYNX", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace impl_;
			return cpmv<CPMVOperation::CP_NX>(p, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"copynx",	"COPYNX"
		};

	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "copy";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				COPY		,
				COPYNX		,
				RENAME		,
				RENAMENX
			>(pack);
		}
	};


} // namespace


