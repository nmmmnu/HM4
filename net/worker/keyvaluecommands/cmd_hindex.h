#include "base.h"

#include "shared_mset_multi.h"

#include "stringtokenizer.h"
#include "utf8slidingwindow.h"

namespace net::worker::commands::HybridIndex{
	namespace hybrid_index_impl_{

		constexpr uint8_t NGram		=  3;
		constexpr size_t  MaxTokens	= 32;

		template<typename DBAdapter, typename SlidingWindow>
		struct MDecoder : shared::msetmulti::FTS::BaseMDecoder<DBAdapter>{

			constexpr static bool checkSize(size_t size){
				auto const sizeNGram = NGram * UTF8Tokenizer::MAX_UTF8_SIZE + 1;

				// Single token is 3 * 4 bytes + 1 ASCII separator = 13 bytes max.
				// We have up to 64K tokens = 13 * 64K = 832KB max.
				// The trigrams can be repeaded, for example 'aaaaaaaaaaaa',
				// but the container needs to hold them before sorting.

				auto const sizeTotal_1 = sizeNGram * OutputBlob::ContainerSize;

				// We also have to store the tokens, their size is same as ngrams
				auto const sizeTotal   = sizeTotal_1 * 2;

				static_assert(
					hm4::Pair::isValValid(sizeTotal)
				);

				return size <= sizeTotal;
			}

			template<typename Container>
			static bool indexesStore(std::string_view value, char separator, Container &container){
				return process__<1>(value, separator, container);
			}

			// template<typename Container>
			// static bool indexesSearch(std::string_view value, char separator, Container &container){
			// 	return process__<0>(value, separator, container);
			// }

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

				for(auto const &x : tok)
					if (! trigramsExplode__(x, container))
						return false;

				if (container.empty())
					return false; // need to have at least one token

				// all tokens are in now

				if constexpr(hasSortKey){
					if (container.full())
						return false; // no room for the sort key
				}

				return true;
			}

			template<typename Container>
			static bool trigramsExplode__(std::string_view value, Container &container){
				SlidingWindow sw{ value, NGram };

				size_t count = 0;

				// insert all trigrams
				for(auto const &x : sw){
					if (!container.full()){
						container.push_back(x);
						++count;
					}else
						return false; // no room for the trigram
				}

				// insert complementary value
				// count == 0 small token
				// count == 1 exact trigram (skip complementary)
				// count >= 2 big   token

				if (count != 1 && !value.empty()){
					auto const x = value;

					if (!container.full())
						container.push_back(x);
					else
						return false; // no room for the trigram
				}

				return true;
			}

			using shared::msetmulti::FTS::BaseMDecoder<DBAdapter>::sort__;
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
			using namespace hybrid_index_impl_;

			using MySlidingWindow	= UTF8SlidingWindow;
			using MyMDecoder	= MDecoder<DBAdapter, MySlidingWindow>;

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
			using namespace hybrid_index_impl_;

			using MySlidingWindow	= BinarySlidingWindow;
			using MyMDecoder	= MDecoder<DBAdapter, MySlidingWindow>;

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

