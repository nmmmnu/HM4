#include "base.h"

#include "shared_accumulateresults.h"
#include "shared_mset_multi.h"

#include "stringtokenizer.h"
#include "utf8slidingwindow.h"
#include "pair_vfactory.h"
#include "ilist/txguard.h"

namespace net::worker::commands::SIndex{
	namespace mindex_impl_{

		constexpr uint8_t NGram		= 3;
		constexpr size_t  MaxTokens	= 32;

		template<typename DBAdapter, typename SlidingWindow>
		struct MDecoder : shared::msetmulti::FTS::BaseMDecoder<DBAdapter>{

			template<typename Container>
			static bool indexesUser(std::string_view value, char, Container &container){
				if (!indexesUserExplode__(value, container))
					return false;

				return indexesUserSort__(container);
			}

		private:
			template<typename Container>
			static bool indexesUserExplode__(std::string_view value, Container &container){
				SlidingWindow sw{ value, NGram };

				for(auto const &x : sw){
					if (container.full())
						return false;

					// if (!checkF(x))
					//	return false;

					container.push_back(x);
				}

				// inside must be: at least one token
				return !container.empty();
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


		// IXSADD_BIN a keySub delimiter "words,words" sort value

		template<typename MyMDecoder, typename Result, typename DBAdapter>
		void processADD(ParamContainer const &p, DBAdapter &db, Result &result, OutputBlob &blob){
			namespace PM = shared::msetmulti;

			if (p.size() != 6)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_6);

			auto const &keyN	= p[1];
			auto const &keySub	= p[2];
			auto const delimiter	= 'x'; // not used here, but have to be passed down...
			auto const &tokens	= p[3];
			auto const &keySort	= p[4];
			auto const &value	= p[5];

			if (!PM::valid(keyN, keySub, keySort))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const gram = mindex_impl_::NGram;

			auto const vGramSize = (value.size() + 1) * gram + 1;

			// if this is OK, then later, when we apply the system delimiter and store it, it will be OK too.
			if (!hm4::Pair::isValValid(vGramSize + keySort.size() + 1))
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			auto &containerNew	= blob.construct<OutputBlob::Container>();
			auto &containerOld	= blob.construct<OutputBlob::Container>();
			auto &buferVal		= blob.allocate<hm4::PairBufferVal>();


			bool const b = shared::msetmulti::add<MyMDecoder>(db, keyN, keySub, tokens, delimiter, keySort, value,
								containerNew, containerOld, buferVal);

			if (!b)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			return result.set_1();
		}

		// IXSRANGESTRICT_BIN key delimiter "words words" count from

		template<typename MyMDecoder, typename MyFTS, typename Result, typename DBAdapter>
		void processFTS(ParamContainer const &p, DBAdapter &db, Result &result, OutputBlob &blob){
			using namespace shared::accumulate_results;
			namespace PM = shared::msetmulti;

			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_5);

			auto const keyN		= p[1];
			auto const delimiter	= 'x'; // not used here, but have to be passed down...
			auto const tokens	= p[2];

			if (keyN.empty() || tokens.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (tokens.size() > (MaxTokens - NGram + 1) * NGram)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const count	= myClamp<uint32_t>(p[3], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const keyStart	= p[4];

			return shared::msetmulti::FTS::processFTS<MyMDecoder, MyFTS>(keyN, delimiter, tokens, count, keyStart,
									db, result, blob);
		}

	} // namespace mindex_impl_



	template<class Protocol, class DBAdapter>
	struct IXSADD_UTF8 : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXSADD_UTF8 a keySub delimiter "words,words" sort value

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return mindex_impl_::processADD<MyMDecoder>(p, db, result, blob);
		}

	private:
		using MySlidingWindow	= UTF8SlidingWindow;
		using MyMDecoder	= mindex_impl_::MDecoder<DBAdapter, MySlidingWindow>;

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixsadd_utf8"	,	"IXSADD_UTF8"	,
			"ixsadd"	,	"IXSADD"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXSADD_BIN : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXSADD_BIN a keySub delimiter "words,words" sort value

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return mindex_impl_::processADD<MyMDecoder>(p, db, result, blob);
		}

	private:
		using MySlidingWindow	= BinarySlidingWindow;
		using MyMDecoder	= mindex_impl_::MDecoder<DBAdapter, MySlidingWindow>;

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixsadd_bin"	,	"IXSADD_BIN"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXSGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXSGET key subkey

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
		using MyMDecoder	= shared::msetmulti::FTS::BaseMDecoder<DBAdapter>;

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixsget"	,	"IXSGET"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXSMGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXSMGET key subkey0 subkey1 ...
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
		using MyMDecoder	= shared::msetmulti::FTS::BaseMDecoder<DBAdapter>;

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixsmget"	,	"IXSMGET"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXSEXISTS : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXSEXISTS key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return shared::msetmulti::cmdProcessExists(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixsexists"	,	"IXSEXISTS"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXSGETINDEXES : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXSGETIXES key subkey

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
		using MyMDecoder	= shared::msetmulti::FTS::BaseMDecoder<DBAdapter>;

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixsgetindexes"	,	"IXSGETINDEXES"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXSREM : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXSDEL a subkey0 subkey1 ...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return shared::msetmulti::cmdProcessRem<MyMDecoder>(p, db, result, blob);
		}

	private:
		using MyMDecoder	= shared::msetmulti::FTS::BaseMDecoder<DBAdapter>;

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixsrem"	,	"IXSREM"	,
			"ixsremove"	,	"IXSREMOVE"	,
			"ixsdel"	,	"IXSDEL"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXSRANGEFLEX_UTF8 : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXSRANGEFLEX_UTF8 key "words words" count from

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return mindex_impl_::processFTS<MyMDecoder, MyFTS>(p, db, result, blob);
		}

	private:
		constexpr static size_t MaxTokens  = mindex_impl_::MaxTokens;

		using MySlidingWindow	= UTF8SlidingWindow;
		using MyMDecoder	= mindex_impl_::MDecoder<DBAdapter, MySlidingWindow>;
		using MyFTS		= shared::msetmulti::FTS::FTSFlex<DBAdapter, MaxTokens>;

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixsrangeflex_utf8"	,	"IXSRANGEFLEX_UTF8"	,
			"ixsrangeflex"		,	"IXSRANGEFLEX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IXSRANGEFLEX_BIN : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXSRANGEFLEX_BIN key delimiter "words words" count from

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return mindex_impl_::processFTS<MyMDecoder, MyFTS>(p, db, result, blob);
		}

	private:
		constexpr static size_t MaxTokens  = mindex_impl_::MaxTokens;

		using MySlidingWindow	= BinarySlidingWindow;
		using MyMDecoder	= mindex_impl_::MDecoder<DBAdapter, MySlidingWindow>;
		using MyFTS		= shared::msetmulti::FTS::FTSFlex<DBAdapter, MaxTokens>;

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixsrangeflex_bin"	,	"IXSRANGEFLEX_BIN"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IXSRANGESTRICT_UTF8 : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXSRANGESTRICT_UTF8 key delimiter "words words" count from

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return mindex_impl_::processFTS<MyMDecoder, MyFTS>(p, db, result, blob);
		}

	private:
		constexpr static size_t MaxTokens  = mindex_impl_::MaxTokens;

		using MySlidingWindow	= UTF8SlidingWindow;
		using MyMDecoder	= mindex_impl_::MDecoder<DBAdapter, MySlidingWindow>;
		using MyFTS		= shared::msetmulti::FTS::FTSStrict<DBAdapter, MaxTokens>;

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixsrangestrict_utf8"	,	"IXSRANGESTRICT_UTF8"	,
			"ixsrangestrict"	,	"IXSRANGESTRICT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IXSRANGESTRICT_BIN : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXSRANGESTRICT_BIN key delimiter "words words" count from

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return mindex_impl_::processFTS<MyMDecoder, MyFTS>(p, db, result, blob);
		}

	private:
		constexpr static size_t MaxTokens  = mindex_impl_::MaxTokens;

		using MySlidingWindow	= BinarySlidingWindow;
		using MyMDecoder	= mindex_impl_::MDecoder<DBAdapter, MySlidingWindow>;
		using MyFTS		= shared::msetmulti::FTS::FTSStrict<DBAdapter, MaxTokens>;

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixsrangestrict_bin"	,	"IXSRANGESTRICT_BIN"
		};
	};





	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "mindex";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				IXSADD_UTF8		,
				IXSADD_BIN		,
				IXSGET			,
				IXSMGET			,
				IXSEXISTS		,
				IXSGETINDEXES		,
				IXSREM			,
				IXSRANGEFLEX_UTF8	,
				IXSRANGEFLEX_BIN	,
				IXSRANGESTRICT_UTF8	,
				IXSRANGESTRICT_BIN
			>(pack);
		}
	};




} // namespace


