#include "base.h"

#include "shared_accumulateresults.h"
#include "shared_mset_multi.h"

#include "stringtokenizer.h"
#include "pair_vfactory.h"
#include "ilist/txguard.h"

namespace net::worker::commands::MIndex{
	namespace mindex_impl_{

		template<typename DBAdapter>
		struct MDecoder : shared::msetmulti::FTS::BaseMDecoder<DBAdapter>{

			template<typename Container>
			static bool indexesUser(std::string_view value, char separator, Container &container){
				if (!indexesUserExplode__(value, separator, container))
					return false;

				return indexesUserSort__(container);
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
			namespace PM = shared::msetmulti;

			if (p.size() != 7)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_6);

			auto const &keyN	= p[1];
			auto const &keySub	= p[2];
			auto const &delimiter	= p[3];
			auto const &tokens	= p[4];
			auto const &keySort	= p[5];
			auto const &value	= p[6];

			if (delimiter.size() != 1)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			if (!PM::valid(keyN, keySub, keySort))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			// if this is OK, then later, when we apply the system delimiter and store it, it will be OK too.
			if (!hm4::Pair::isValValid(value.size() + keySort.size() + 1))
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			auto &containerNew	= blob.construct<OutputBlob::Container>();
			auto &containerOld	= blob.construct<OutputBlob::Container>();
			auto &buferVal		= blob.allocate<hm4::PairBufferVal>();

			bool const b = shared::msetmulti::add<MyMDecoder>(db, keyN, keySub, tokens, delimiter[0], keySort, value,
								containerNew, containerOld, buferVal);

			if (!b)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			return result.set_1();
		}

	private:
		using MyMDecoder = mindex_impl_::MDecoder<DBAdapter>;

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixmadd",	"IXMADD"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXMGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXMGET key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			namespace PM = shared::msetmulti;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN   = p[1];
			auto const &keySub = p[2];

			if (!PM::valid(keyN, keySub))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			return result.set(
				PM::get<MyMDecoder>(db, keyN, keySub)
			);
		}

	private:
		using MyMDecoder = shared::msetmulti::FTS::BaseMDecoder<DBAdapter>;

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixmget",	"IXMGET"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXMMGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXMMGET key subkey0 subkey1 ...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			namespace PM = shared::msetmulti;

			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_3);

			const auto &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (auto const &keySub = *itk; !PM::valid(keyN, keySub))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto &container = blob.construct<OutputBlob::Container>();

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &keySub = *itk;

				container.emplace_back(
					PM::get<MyMDecoder>(db, keyN, keySub)
				);
			}

			return result.set_container(container);
		}

	private:
		using MyMDecoder = shared::msetmulti::FTS::BaseMDecoder<DBAdapter>;

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixmmget",	"IXMMGET"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXMEXISTS : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXMEXISTS key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return shared::msetmulti::cmdProcessExists(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixmexists",	"IXMEXISTS"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXMGETINDEXES : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXMGETIXES key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			namespace PM = shared::msetmulti;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN   = p[1];
			auto const &keySub = p[2];

			if (!PM::valid(keyN, keySub))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto &container = blob.construct<OutputBlob::Container>();

			[[maybe_unused]]
			bool const b = PM::getIndexes<MyMDecoder>(db, keyN, keySub, container);

			return result.set_container(container);
		}

	private:
		using MyMDecoder = shared::msetmulti::FTS::BaseMDecoder<DBAdapter>;

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixmgetindexes",	"IXMGETINDEXES"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXMREM : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXMDEL a subkey0 subkey1 ...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return shared::msetmulti::cmdProcessRem<MyMDecoder>(p, db, result, blob);
		}

	private:
		using MyMDecoder = shared::msetmulti::FTS::BaseMDecoder<DBAdapter>;

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixmrem",	"IXMREM",
			"ixmremove",	"IXMREMOVE",
			"ixmdel",	"IXMDEL"
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

			return PM::cmdProcessRange(keyN, index, count, keyStart,
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
			using namespace shared::accumulate_results;
			namespace PM = shared::msetmulti;

			if (p.size() != 6)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_5);

			auto const keyN		= p[1];
			auto const delimiter	= p[2];
			auto const tokens	= p[3];

			if (delimiter.size() != 1)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			if (keyN.empty() || tokens.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const count	= myClamp<uint32_t>(p[4], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const keyStart	= p[5];

			return shared::msetmulti::FTS::processFTS<MyMDecoder, MyFTS, MyTokenContainer>(keyN, delimiter[0], tokens, count, keyStart,
									db, result, blob);
		}

	private:
		constexpr static size_t MaxTokens  = 32;

		using MyTokenContainer	= StaticVector<std::string_view, MaxTokens>;
		using MyMDecoder	= mindex_impl_::MDecoder<DBAdapter>;
		using MyFTS		= shared::msetmulti::FTS::FTSFlex<DBAdapter, MaxTokens>;

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
			using namespace shared::accumulate_results;
			namespace PN = mindex_impl_;

			if (p.size() != 6)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_5);

			auto const keyN		= p[1];
			auto const delimiter	= p[2];
			auto const tokens	= p[3];

			if (delimiter.size() != 1)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			if (keyN.empty() || tokens.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const count	= myClamp<uint32_t>(p[4], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const keyStart	= p[5];

			return shared::msetmulti::FTS::processFTS<MyMDecoder, MyFTS, MyTokenContainer>(keyN, delimiter[0], tokens, count, keyStart,
									db, result, blob);
		}

	private:
		constexpr static size_t MaxTokens  = 32;

		using MyTokenContainer	= StaticVector<std::string_view, MaxTokens>;
		using MyMDecoder	= mindex_impl_::MDecoder<DBAdapter>;
		using MyFTS		= shared::msetmulti::FTS::FTSStrict<DBAdapter, MaxTokens>;

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
				IXMGET		,
				IXMMGET		,
				IXMEXISTS	,
				IXMGETINDEXES	,
				IXMREM		,
				IXMRANGE	,
				IXMRANGEFLEX	,
				IXMRANGESTRICT
			>(pack);
		}
	};




} // namespace


