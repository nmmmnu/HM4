#include "base.h"

#include "shared_mset_multi.h"

#include "stringtokenizer.h"
#include "utf8slidingwindow.h"

namespace net::worker::commands::TIndex{
	namespace tindex_impl_{

		constexpr uint8_t NGram		= 3;
		constexpr size_t  MaxTokens	= 32;

		template<typename DBAdapter, typename SlidingWindow>
		struct MDecoder : shared::msetmulti::FTS::BaseMDecoder<DBAdapter>{

			constexpr static bool checkSize(size_t){
				static_assert(
					hm4::Pair::isValValid(
						(NGram + 1) * MaxTokens * UTF8Tokenizer::MAX_UTF8_SIZE < hm4::PairConf::MAX_VAL_SIZE
					)
				);

				return true;
			}

			// template<typename Container>
			// static bool indexesUser(std::string_view value, char, Container &container){
			// 	if (!indexesUserExplode__(value, container))
			// 		return false;
			//
			// 	return indexesUserSort__(container);
			// }

			template<typename Container>
			static bool indexesUser(std::string_view value, char separator, Container &container){
				if (!indexesFindExplode__(value, separator, container))
					return false;

				return indexesUserSort__(container);
			}

			template<typename Container>
			static bool indexesFind(std::string_view value, char separator, Container &container){
				return indexesUser(value, separator, container);
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
			static bool indexesFindExplode__(std::string_view value, char separator, Container &container){
				StringTokenizer const tok{ value, separator };

				for(auto const &value2 : tok)
					if (! indexesUserExplode__(value2, container) )
						return false;

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

	} // namespace tindex_impl_



	template<class Protocol, class DBAdapter>
	struct IXTADD_UTF8 : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXTADD_UTF8 a keySub delimiter "words,words" sort value

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using MySlidingWindow	= UTF8SlidingWindow;
			using MyMDecoder	= tindex_impl_::MDecoder<DBAdapter, MySlidingWindow>;

			return shared::msetmulti::cmdProcessAdd<MyMDecoder>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixtadd_utf8"	,	"IXTADD_UTF8"	,
			"ixtadd"	,	"IXTADD"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXTADD_BIN : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXTADD_BIN a keySub delimiter "words,words" sort value

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using MySlidingWindow	= BinarySlidingWindow;
			using MyMDecoder	= tindex_impl_::MDecoder<DBAdapter, MySlidingWindow>;

			return shared::msetmulti::cmdProcessAdd<MyMDecoder>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixtadd_bin"	,	"IXTADD_BIN"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXTGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXTGET key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return shared::msetmulti::cmdProcessGet(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixtget"	,	"IXTGET"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXTMGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXTMGET key subkey0 subkey1 ...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return shared::msetmulti::cmdProcessMGet(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixtmget"	,	"IXTMGET"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXTEXISTS : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXTEXISTS key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return shared::msetmulti::cmdProcessExists(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixtexists"	,	"IXTEXISTS"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXTGETINDEXES : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXTGETINDEXES key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return shared::msetmulti::cmdProcessGetIndexes(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixtgetindexes"	,	"IXTGETINDEXES"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXTREM : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXTDEL a subkey0 subkey1 ...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return shared::msetmulti::cmdProcessRem(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixtrem"	,	"IXTREM"	,
			"ixtremove"	,	"IXTREMOVE"	,
			"ixtdel"	,	"IXTDEL"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXTRANGEFLEX_UTF8 : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXTRANGEFLEX_UTF8 key "words words" count from

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace tindex_impl_;

			using MySlidingWindow	= UTF8SlidingWindow;
			using MyMDecoder	= MDecoder<DBAdapter, MySlidingWindow>;
			using MyFTS		= shared::msetmulti::FTS::FTSFlex<DBAdapter, MaxTokens>;

			return shared::msetmulti::cmdProcessRangeMulti<MyMDecoder, MyFTS>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixtrangeflex_utf8"	,	"IXTRANGEFLEX_UTF8"	,
			"ixtrangeflex"		,	"IXTRANGEFLEX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IXTRANGEFLEX_BIN : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXTRANGEFLEX_BIN key delimiter "words words" count from

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace tindex_impl_;

			using MySlidingWindow	= BinarySlidingWindow;
			using MyMDecoder	= MDecoder<DBAdapter, MySlidingWindow>;
			using MyFTS		= shared::msetmulti::FTS::FTSFlex<DBAdapter, MaxTokens>;

			return shared::msetmulti::cmdProcessRangeMulti<MyMDecoder, MyFTS>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixtrangeflex_bin"	,	"IXTRANGEFLEX_BIN"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IXTRANGESTRICT_UTF8 : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXTRANGESTRICT_UTF8 key delimiter "words words" count from

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace tindex_impl_;

			using MySlidingWindow	= UTF8SlidingWindow;
			using MyMDecoder	= MDecoder<DBAdapter, MySlidingWindow>;
			using MyFTS		= shared::msetmulti::FTS::FTSStrict<DBAdapter, MaxTokens>;

			return shared::msetmulti::cmdProcessRangeMulti<MyMDecoder, MyFTS>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixtrangestrict_utf8"	,	"IXTRANGESTRICT_UTF8"	,
			"ixtrangestrict"	,	"IXTRANGESTRICT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IXTRANGESTRICT_BIN : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXTRANGESTRICT_BIN key delimiter "words words" count from

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace tindex_impl_;

			using MySlidingWindow	= BinarySlidingWindow;
			using MyMDecoder	= MDecoder<DBAdapter, MySlidingWindow>;
			using MyFTS		= shared::msetmulti::FTS::FTSStrict<DBAdapter, MaxTokens>;

			return shared::msetmulti::cmdProcessRangeMulti<MyMDecoder, MyFTS>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixtrangestrict_bin"	,	"IXTRANGESTRICT_BIN"
		};
	};





	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "mindex";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				IXTADD_UTF8		,
				IXTADD_BIN		,
				IXTGET			,
				IXTMGET			,
				IXTEXISTS		,
				IXTGETINDEXES		,
				IXTREM			,
				IXTRANGEFLEX_UTF8	,
				IXTRANGEFLEX_BIN	,
				IXTRANGESTRICT_UTF8	,
				IXTRANGESTRICT_BIN
			>(pack);
		}
	};




} // namespace


