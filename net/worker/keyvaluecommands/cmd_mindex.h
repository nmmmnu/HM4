#include "base.h"

#include "shared_mset_multi.h"
#include "stringtokenizer.h"

namespace net::worker::commands::MIndex{
	namespace mindex_impl_{

		// constexpr uint8_t NGram	= 3;
		constexpr size_t  MaxTokens	= 32;

		template<typename DBAdapter>
		struct MDecoder : shared::msetmulti::FTS::BaseMDecoder<DBAdapter>{

			constexpr static bool checkSize(size_t size){
				return hm4::Pair::isValValid(size);
			}

			template<typename Container>
			static bool indexesUser(std::string_view value, char separator, Container &container){
				if (!indexesUserExplode__(value, separator, container))
					return false;

				return indexesUserSort__(container);
			}

			template<typename Container>
			static bool indexesFind(std::string_view value, char separator, Container &container){
				return indexesUser(value, separator, container);
			}

		private:
			template<typename Container>
			static bool indexesUserExplode__(std::string_view value, char separator, Container &container){
				StringTokenizer const tok{ value, separator };

				size_t count = 0;

				for(auto const &x : tok){
					// we need to have space for the sort
					if (++count >= container.capacity())
						return false;

					// if (!checkF(x))
					// 	return false;

					container.push_back(x);
				}

				return true;
			}

			template<typename Container>
			static bool indexesUserSort__(Container &container){
				std::sort(std::begin(container), std::end(container));

				#if 0
					container.erase(
						std::unique( std::begin(container), std::end(container) ),
						std::end(container)
					);
				#else
					// Quick fix for StaticVector et all

					if (auto it = std::unique(std::begin(container), std::end(container)); it != std::end(container))
						while (container.end() != it)
							container.pop_back();
				#endif

				return true;
			}
		};

	} // namespace mindex_impl_



	template<class Protocol, class DBAdapter>
	struct IXMADD : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXMADD a keySub delimiter "words,words" sort value

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using MyMDecoder = mindex_impl_::MDecoder<DBAdapter>;

			return shared::msetmulti::cmdProcessAdd<MyMDecoder>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixmadd",	"IXMADD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IXMRANGE : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXMRANGE key txt count from

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
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
		constexpr inline static std::string_view cmd[]	= {
			"ixmrange",	"IXMRANGE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IXMRANGEFLEX : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXMRANGEFLEX key delimiter "words,words" count from

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace mindex_impl_;

			using MyMDecoder	= MDecoder<DBAdapter>;
			using MyFTS		= shared::msetmulti::FTS::FTSFlex<DBAdapter, MaxTokens>;

			return shared::msetmulti::cmdProcessRangeMulti<MyMDecoder, MyFTS>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixmrangeflex",	"IXMRANGEFLEX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IXMRANGESTRICT : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXMRANGESTRICT key delimiter "words,words" count from

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace mindex_impl_;

			constexpr static size_t MaxTokens  = 32;

			using MyMDecoder	= MDecoder<DBAdapter>;
			using MyFTS		= shared::msetmulti::FTS::FTSStrict<DBAdapter, MaxTokens>;

			return shared::msetmulti::cmdProcessRangeMulti<MyMDecoder, MyFTS>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixmrangestrict",	"IXMRANGESTRICT"
		};
	};





	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "mindex";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				IXMADD		,
			//	IXMGET		,
			//	IXMMGET		,
			//	IXMEXISTS	,
			//	IXMGETINDEXES	,
			//	IXMREM		,
				IXMRANGE	,
				IXMRANGEFLEX	,
				IXMRANGESTRICT
			>(pack);
		}
	};




} // namespace


