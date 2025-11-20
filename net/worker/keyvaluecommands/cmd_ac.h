#include "base.h"

#include "stringtokenizer.h"
#include "pair_vfactory.h"
#include "ilist/txguard.h"

#include "utf8tokenizer.h"
#include "checkoverflow.h"

#include "shared_zset_multi.h"
#include "shared_accumulateresults.h"

namespace net::worker::commands::AC{
	namespace ac_impl_{
		using namespace net::worker::shared::accumulate_results;
		using namespace net::worker::shared::config;
		using P1 = net::worker::shared::zsetmulti::Permutation1NoIndex;



		template<typename T, typename List>
		inline T getValueN(List &list, std::string_view key){
			auto const s = hm4::getPairVal(list, key);

			auto const n1 = from_string<T>(s);

			auto const n2 = safe_overflow::incr(n1, T{1});

			return n2;
		}

		// ACADD_XXX a text exp

		template<typename Tokenizer, class Protocol, class DBAdapter>
		void do_processACADD(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 3 && p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_23);

			auto const &keyN	= p[1];
			auto const &text	= p[2];
			auto const expire	= p.size() > 3 ? from_string<uint32_t>(p[3]) : 0;

			if (!P1::valid(keyN, text, { text } ))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			Tokenizer tok{ text };

			hm4::PairBufferKey bufferKey;
			auto const key = P1::makeKeyData(bufferKey, DBAdapter::SEPARATOR,
					keyN		,
					text		,
					""
			);

			auto const valueN = getValueN<uint64_t>(*db, key);

			to_string_buffer_t bufferN;
			auto const value  = to_string(valueN, bufferN);

			hm4::insert(*db, key, value, expire);

			for(auto it = std::begin(tok); it != std::end(tok); ++it){
				auto const key = P1::makeKeyData(bufferKey, DBAdapter::SEPARATOR,
						keyN		,
						text		,
						tok.to(it)
				);

				hm4::insert(*db, key, value, expire);
			}

			return result.set(valueN);
		}

		// ACDEL_XXX a text

		template<typename Tokenizer, class Protocol, class DBAdapter>
		void do_processACDEL(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_23);

			auto const &keyN	= p[1];
			auto const &text	= p[2];

			if (!P1::valid(keyN, text, { text } ))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			Tokenizer tok{ text };

			hm4::PairBufferKey bufferKey;
			auto const key = P1::makeKeyData(bufferKey, DBAdapter::SEPARATOR,
					keyN		,
					text		,
					""
			);

			hm4::erase(*db, key);

			for(auto it = std::begin(tok); it != std::end(tok); ++it){
				auto const key = P1::makeKeyData(bufferKey, DBAdapter::SEPARATOR,
						keyN		,
						text		,
						tok.to(it)
				);

				hm4::erase(*db, key);
			}

			return result.set(true);
		}



		inline std::string_view extractNth(size_t const nth, char const separator, std::string_view const s){
			size_t count = 0;

			for (size_t i = 0; i < s.size(); ++i)
				if (s[i] == separator)
					if (++count; count == nth)
						return s.substr(i + 1);

			return "INVALID_DATA";
		}



		inline std::string_view makeKeyDataSearch(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
						std::string_view txt
				){

			// keyN~word~

			return concatenateBuffer(bufferKey,
					keyN	,	separator	,
					txt	,	separator
			);
		}



		template<typename DBAdapter>
		class AC{
		public:
			AC(DBAdapter &db, std::string_view keyN, std::string_view text, uint32_t resultCount, std::string_view keyStart) :
								db_		(db		),
								keyN_		(keyN		),
								text_		(text		),
								resultCount_	(resultCount	),
								keyStart_	(keyStart	){}

			void operator()(OutputBlob::Container &container){
				auto const tail = populateHeap_();

				std::sort_heap(std::begin(heap_), std::end(heap_), & Result::comp);

				// collecting the data
				container.clear();

				for(auto const &r : heap_){
					logger<Logger::DEBUG>() << "ACRANGE" << r.key << r.val << r.num;

					container.push_back(r.key);
					container.push_back(r.val);
				}

				// --------------------------

				container.emplace_back(tail);
			}

		private:
			std::string_view populateHeap_(){
				std::string_view tail;

				auto pTail = [&tail](std::string_view x = ""){
					tail = x;
				};

				auto pPair = [&](auto const &pair) -> bool{

					auto projKey = [](std::string_view x){
						auto const separator = DBAdapter::SEPARATOR[0];

						// keyN~word~
						return extractNth(2, separator, x);
					};

					auto const pkey = projKey(pair.getKey());
					auto const val  = pair.getVal();
					auto const num  = from_string<uint32_t>(val);

					if (heap_.size() < resultCount_){
						heap_.emplace_back(pkey, val, num);
						std::push_heap(std::begin(heap_), std::end(heap_), & Result::comp);

						return true;
					}

					if (!Result::compN(num, heap_.front()))
						return true;

					// remove smallest
					std::pop_heap(std::begin(heap_), std::end(heap_), & Result::comp);
					heap_.pop_back();

					// insert bigger
					heap_.emplace_back(pkey, val, num);
					std::push_heap(std::begin(heap_), std::end(heap_), & Result::comp);

					return true;
				};

				hm4::PairBufferKey bufferKey;
				auto const prefix = makeKeyDataSearch(bufferKey, DBAdapter::SEPARATOR, keyN_, text_);
				auto const key = keyStart_.empty() ? prefix : keyStart_;

				StopPrefixPredicate stop{ prefix };

				sharedAccumulatePairs(
					stop		,
					db_->find(key)	,
					db_->end()	,
					pTail		,
					pPair
				);

				return tail;
			}

		private:
			struct Result{
				std::string_view	key;
				std::string_view	val;
				uint64_t		num;

				constexpr Result(std::string_view key, std::string_view val, uint64_t num) :
								key(key),
								val(val),
								num(num){}

				constexpr static bool comp(Result const &a, Result const &b){
					return a.num > b.num;
				}

				constexpr static bool compN(uint64_t num, Result const &b){
					return num > b.num;
				}
			};

			using ResultHeap	= StaticVector<Result, ITERATIONS_RESULTS_MAX>;

		private:
			DBAdapter		&db_		;
			std::string_view	keyN_		;
			std::string_view	text_		;
			uint64_t		resultCount_	;
			std::string_view	keyStart_	;

			ResultHeap		heap_		;
		};

	} // namespace mindex_impl_



	template<class Protocol, class DBAdapter>
	struct ACADD_UTF8 : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// ACADD_UTF8 a text exp

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace ac_impl_;

			return do_processACADD<UTF8Tokenizer>(p, db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"acadd_utf8",	"ACADD_UTF8"	,	"acadd",	"ACADD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct ACADD_BIN : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// ACADD_BIN a text exp

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace ac_impl_;

			return do_processACADD<ASCIITokenizer>(p, db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"acadd_bin",	"ACADD_BIN"
		};
	};



	template<class Protocol, class DBAdapter>
	struct ACDEL_UTF8 : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// ACADD_UTF8 a text exp

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace ac_impl_;

			return do_processACDEL<UTF8Tokenizer>(p, db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"acdel_utf8",		"ACDEL_UTF8"	,	"acdel",	"ACDEL"		,
			"acrem_utf8",		"ACREM_UTF8"	,	"acrem",	"ACREM"		,
			"acremove_utf8",	"ACREMOVE_UTF8"	,	"acremove",	"ACREMOVE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct ACDEL_BIN : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// ACADD_BIN a text exp

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace ac_impl_;

			return do_processACDEL<ASCIITokenizer>(p, db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"acdel_bin",		"ACDEL_BIN"	,
			"acrem_bin",		"ACREM_BIN"	,
			"acremove_bin",		"ACREMOVE_BIN"
		};
	};



	template<class Protocol, class DBAdapter>
	struct ACRANGE : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// ACRANGE a text count keyStart

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace ac_impl_;

			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_23);

			auto const &keyN	= p[1];
			auto const &text	= p[2];	// can be empty

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const resultCount	= myClamp<uint32_t>(p[3], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const keyStart	= p[4];

			using MyAC = AC<DBAdapter>;

			auto &ac = blob.construct<MyAC>(db, keyN, text, resultCount, keyStart);

			auto &container = blob.construct<OutputBlob::Container>();

			ac(container);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"acrange",		"ACRANGE"	,
			"acrange_bin",		"ACRANGE_BIN"	,
			"acrange_utf8",		"ACRANGE_UTF8"
		};
	};



	template<class Protocol, class DBAdapter>
	struct ACRANGEALL : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// ACRANGEALL a count keyStart

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace ac_impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_23);

			auto const &keyN	= p[1];
			auto const text		= "";

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const resultCount	= myClamp<uint32_t>(p[2], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const keyStart	= p[3];

			using MyAC = AC<DBAdapter>;

			auto &ac = blob.construct<MyAC>(db, keyN, text, resultCount, keyStart);

			auto &container = blob.construct<OutputBlob::Container>();

			ac(container);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"acrangeall",		"ACRANGEALL"	,
			"acrangeall_bin",	"ACRANGEALL_BIN"	,
			"acrangeall_utf8",	"ACRANGEALL_UTF8"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "autocomplete";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				ACADD_UTF8	,	ACDEL_UTF8	,
				ACADD_BIN	,	ACDEL_BIN	,
				ACRANGE		,
				ACRANGEALL
			>(pack);
		}
	};



} // namespace







#if 0
			std::string_view populateHeap____(){
				uint32_t iterations	= 0;
				uint32_t results	= 0;

				auto proj = [](std::string_view x){
					auto const separator = DBAdapter::SEPARATOR[0];

					// keyN~word~
					return extractNth(2, separator, x);
				};

				hm4::PairBufferKey bufferKey;
				auto const prefix = makeKeyDataSearch(bufferKey, DBAdapter::SEPARATOR, keyN_, text_);
				auto const key = keyStart_.empty() ? prefix : keyStart_;

				StopPrefixPredicate stop{ prefix };

				logger<Logger::DEBUG>() << "ACRANGE prefix" << prefix;

				for(auto it = db_->find(key); it != db_->end(); ++it){
					auto const &key = it->getKey();

					auto const pkey = proj(key);

					logger<Logger::DEBUG>() << "ACRANGE keys" << key << pkey;

					if (++iterations > ITERATIONS_LOOPS_MAX)
						return key; // tail

					if (stop(key))
						return ""; // no tail

					if (! it->isOK())
						continue;

					if (++results > resultCount_)
						return key; // tail

					auto const &val = it->getVal();
					auto const num  = from_string<uint32_t>(val);

					if (heap_.size() < resultCount_){
						heap_.emplace_back(pkey, val, num);
						std::push_heap(std::begin(heap_), std::end(heap_), & Result::comp);

						logger<Logger::DEBUG>() << "ACRANGE add to heap" << pkey << val << num;

						continue;
					}

					if (!Result::compN(num, heap_.front()))
						continue;

					// remove smallest
					std::pop_heap(std::begin(heap_), std::end(heap_), & Result::comp);
					heap_.pop_back();

					// insert bigger
					heap_.emplace_back(pkey, val, num);
					std::push_heap(std::begin(heap_), std::end(heap_), & Result::comp);
				}

				return ""; // no tail
			}
#endif

