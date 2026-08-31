#include "base.h"

#include "shared_mset_multi.h"
#include "stringtokenizer.h"

namespace net::worker::commands::MultiIndex{
	namespace impl_{

		// constexpr uint8_t NGram	=  3;
		constexpr size_t  MaxTokens	= 32;

		template<typename DBAdapter>
		struct MDecoder : shared::msetmulti::FTS::BaseMDecoder<DBAdapter>{

			constexpr static bool checkSize(size_t size){
				return hm4::Pair::isValValid(size);
			}

			template<typename Container>
			static bool indexesStore(std::string_view value, char separator, Container &container){
				return process__<1>(value, separator, container);
			}

			template<typename Container>
			static bool indexesSearch(std::string_view value, char separator, Container &container){
				return process__<0>(value, separator, container);
			}

		private:
			template<bool hasSortKey, typename Container>
			static bool process__(std::string_view value, char separator, Container &container){
				if (!tokensExplode__<hasSortKey>(value, separator, container))
					return false;

				return sort__(container);
			}

			template<bool hasSortKey, typename Container>
			static bool tokensExplode__(std::string_view value, char separator, Container &container){
				StringTokenizer const tok{ value, separator };

				for(auto const &x : tok){
					if (!container.full())
						container.push_back(x);
					else
						return false; // no room for the token
				}

				if (container.empty())
					return false; // need to have at least one token

				// all tokens are in now

				if constexpr(hasSortKey){
					if (container.full())
						return false; // no room for the sort key
				}

				return true;
			}

			using shared::msetmulti::FTS::BaseMDecoder<DBAdapter>::sort__;
		};

	} // namespace impl_



	template<class Protocol, class DBAdapter>
	struct IXMADD : BaseCommandRW<Protocol,DBAdapter>{

		IXMADD() : BaseCommandRW<Protocol,DBAdapter>("IXMADD", std::begin(cmd__), std::end(cmd__)){}

		// IXMADD a keySub delimiter "words,words" sort value

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace impl_;

			using MyMDecoder = MDecoder<DBAdapter>;

			return shared::msetmulti::cmdProcessAdd<MyMDecoder>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ixmadd",	"IXMADD"
		};

	};



	template<class Protocol, class DBAdapter>
	struct IXMRANGE : BaseCommandRO<Protocol,DBAdapter>{

		IXMRANGE() : BaseCommandRO<Protocol,DBAdapter>("IXMRANGE", std::begin(cmd__), std::end(cmd__)){}

		// IXMRANGE key txt count from

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using namespace shared::accumulate_results;
			namespace PM = shared::msetmulti;

			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			auto const keyN     = p[1];
			auto const index    = p[2];

			if (keyN.empty() || index.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!PM::valid(keyN, index))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const count    = myClamp<uint32_t>(p[3], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const keyStart = p[4];

			return PM::rangeSingle(keyN, index, count, keyStart,
									db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ixmrange",	"IXMRANGE"
		};

	};



	template<class Protocol, class DBAdapter>
	struct IXMRANGEFLEX : BaseCommandRO<Protocol,DBAdapter>{

		IXMRANGEFLEX() : BaseCommandRO<Protocol,DBAdapter>("IXMRANGEFLEX", std::begin(cmd__), std::end(cmd__)){}

		// IXMRANGEFLEX key delimiter "words,words" count from

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace impl_;

			using MyMDecoder	= MDecoder<DBAdapter>;
			using MyFTS		= shared::msetmulti::FTS::FTSFlex<DBAdapter, MaxTokens>;

			return shared::msetmulti::cmdProcessRangeMulti<MyMDecoder, MyFTS>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ixmrangeflex",	"IXMRANGEFLEX"
		};

	};



	template<class Protocol, class DBAdapter>
	struct IXMRANGESTRICT : BaseCommandRO<Protocol,DBAdapter>{

		IXMRANGESTRICT() : BaseCommandRO<Protocol,DBAdapter>("IXMRANGESTRICT", std::begin(cmd__), std::end(cmd__)){}

		// IXMRANGESTRICT key delimiter "words,words" count from

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace impl_;

			using MyMDecoder	= MDecoder<DBAdapter>;
			using MyFTS		= shared::msetmulti::FTS::FTSStrict<DBAdapter, MaxTokens>;

			return shared::msetmulti::cmdProcessRangeMulti<MyMDecoder, MyFTS>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ixmrangestrict",	"IXMRANGESTRICT"
		};

	};





	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "mindex";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				IXMADD		,
				IXMRANGE	,
				IXMRANGEFLEX	,
				IXMRANGESTRICT
			>(pack);
		}
	};




} // namespace


