#include "base.h"

#include "shared_mset_multi.h"

#include "stringtokenizer.h"
#include "utf8slidingwindow.h"

namespace net::worker::commands::HybridIndex{
	namespace hybrid_index_impl_{

		constexpr uint8_t NGram		= 3;
		constexpr size_t  MaxTokens	= 32;

		template<typename DBAdapter, typename SlidingWindow>
		struct MDecoder : shared::msetmulti::FTS::BaseMDecoder<DBAdapter>{

			constexpr static bool checkSize(size_t size){
				static_assert(
					hm4::Pair::isValValid(
						size +
						(NGram + 1) * MaxTokens * UTF8Tokenizer::MAX_UTF8_SIZE < hm4::PairConf::MAX_VAL_SIZE
					)
				);

				return true;
			}

			template<typename Container>
			static bool indexesUser(std::string_view value, char separator, Container &container){
				if (!indexesFindExplode__(value, separator, container))
					return false;

				return indexesUserSort__(container);
			}

			// template<typename Container>
			// static bool indexesFind(std::string_view value, char separator, Container &container){
			// 	return indexesUser(value, separator, container);
			// }

		private:
			template<typename Container>
			static bool indexesFindExplode__(std::string_view value, char separator, Container &container){
				StringTokenizer const tok{ value, separator };

				for(auto const &value2 : tok)
					if (! indexesUserExplode__(value2, container) )
						return false;

				return true;
			}

			template<typename Container>
			static bool indexesUserExplode__(std::string_view value, Container &container){
				SlidingWindow sw{ value, NGram };

				// add TIndex

				for(auto const &x : sw){
					if (container.full())
						return false;

					// if (!checkF(x))
					//	return false;

					container.push_back(x);
				}

				// add MIndex
				container.push_back(value);

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

	} // namespace tindex_impl_



	template<class Protocol, class DBAdapter>
	struct IXHADD_UTF8 : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXTADD_UTF8 a keySub delimiter "words,words" sort value

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using MySlidingWindow	= UTF8SlidingWindow;
			using MyMDecoder	= hybrid_index_impl_::MDecoder<DBAdapter, MySlidingWindow>;

			return shared::msetmulti::cmdProcessAdd<MyMDecoder>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixhadd_utf8"	,	"IXHADD_UTF8"	,
			"ixhadd"	,	"IXHADD"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXHADD_BIN : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXTADD_BIN a keySub delimiter "words,words" sort value

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using MySlidingWindow	= BinarySlidingWindow;
			using MyMDecoder	= hybrid_index_impl_::MDecoder<DBAdapter, MySlidingWindow>;

			return shared::msetmulti::cmdProcessAdd<MyMDecoder>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixhadd_bin"	,	"IXHADD_BIN"
		};
	};





	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "mindex";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				IXHADD_UTF8	,
				IXHADD_BIN
			>(pack);
		}
	};




} // namespace

