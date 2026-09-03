#include "base.h"

#include "shared_rset_multi.h"
#include "shared_rset_multi_fts.h"

#include "shared_accumulateresults.h"
#include "shared_extractnth.h"

#include <algorithm>	// copy, accumulate, is_sorted

namespace net::worker::commands::MultiIndex2{
	namespace impl_{

		constexpr size_t  MaxSearchTokens	= 32;

		using SearchTokenContainer		= OutputBlob::TContainer	<MaxSearchTokens	>;
		using SearchTokenBufferKContainer	= OutputBlob::TKContainer	<MaxSearchTokens * 2	>; // for index and keyStart



		template<typename Container>
		bool validateTokensUser(char delimiter, std::string_view tokens, Container &container){
			container.clear();

			StringTokenizer const tok{ tokens, delimiter };

			for(auto const &x : tok){
				if (container.full())
					return false; // no room for the token

				if (x.empty())
					continue;

				container.push_back(x);
			}

			if (container.empty())
				return false; // need to have at least one token

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

		bool validateTokensStored(char delimiter, std::string_view tokens, OutputBlob::Container &container){
			container.clear();

			StringTokenizer const tok{ tokens, delimiter };

			std::string_view prev;

			for(auto const &x : tok){
				if (container.full())
					return false; // no room for the token

				if (x.empty())
					return false;

				if (!prev.empty() && prev >= x)
					return false;

				container.push_back(x);

				prev = x;
			}

			if (container.empty())
				return false; // need to have at least one token

			return true;
		}

		size_t sizeTokens(OutputBlob::Container const &container, std::string_view keySort){
			auto f = [](size_t sum, std::string_view sv){
				return sum + sv.size() + 1;
			};

			size_t const sum = std::accumulate(std::begin(container), std::end(container), size_t{ 0 }, f);

			return sum + keySort.size();
		}



		template<typename Container>
		bool validateTokensUser__withKeySort(char delimiter, std::string_view tokens, std::string_view keySort, Container &container){
			container.clear();

			StringTokenizer const tok{ tokens, delimiter };

			for(auto const &x : tok){
				if (container.full())
					return false; // no room for the token

				if (x.empty())
					continue;

				container.push_back(x);
			}

			if (container.empty())
				return false; // need to have at least one token

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

			container.push_back(keySort);

			return true;
		}

		bool validateTokensStored__withKeySort(char delimiter, std::string_view tokens, OutputBlob::Container &container){
			container.clear();

			StringTokenizer const tok{ tokens, delimiter };

			std::string_view prev;

			for(auto const &x : tok){
				if (container.full())
					return false; // no room for the token

				if (x.empty())
					return false;

				container.push_back(x);

				prev = x;
			}

			if (container.size() < 2)
				return false; // need to have at least one token and keySort

			return std::is_sorted(std::begin(container), std::prev(std::end(container)));
		}



		template<typename DBAdapter>
		struct Decoder{
			constexpr static size_t bytes(){
				return 0;
			}

			template<typename IContainer, typename BContainer>
			bool operator()(std::string_view tokens,
						IContainer &icontainer, BContainer const &) const{

				return validateTokensStored(DBAdapter::SEPARATOR[0], tokens, icontainer);
			}

			template<typename IContainer, typename BContainer>
			bool operator()(std::true_type, std::string_view tokens,
						IContainer &icontainer, BContainer const &) const{

				return validateTokensStored__withKeySort(DBAdapter::SEPARATOR[0], tokens, icontainer);
			}
		};



		template<typename DBAdapter, typename Result>
		void range1(std::string_view keyN, std::string_view index,
							OutputBlob::Container &container,
								uint32_t count, std::string_view keyStart,
										DBAdapter &db, Result &result){
			hm4::PairBufferKey bufferPrefix;
			auto const prefix = shared::rsetmulti::makeKeyDataSearch(bufferPrefix,
											DBAdapter::SEPARATOR,
												keyN, index);

			hm4::PairBufferKey bufferKeyStart;
			auto const key = keyStart.empty() ? prefix :
						shared::rsetmulti::makeKeyDataStart(bufferKeyStart,
											DBAdapter::SEPARATOR,
												keyN, index, keyStart);

			logger<Logger::DEBUG>() << "MultiIndex2::range1" << "prefix" << prefix << "key" << key;

			auto proj = [](std::string_view x){
				// keyN~word~
				return shared::extractnth::extractNth(3, DBAdapter::SEPARATOR[0], x);
			};

			using namespace shared::accumulate_results;

			StopPrefixPredicate stop{ prefix };

			auto const Out = AccumulateOutput::KEYS_WITH_TAIL;

			sharedAccumulateResults<Out>(
				count		,
				stop		,
				db->find(key)	,
				std::end(*db)	,
				container	,
				proj
			);

			if (auto &next = container.back(); !next.empty())
				next = shared::extractnth::extractNth(2, DBAdapter::SEPARATOR[0], next);

			return result.set_container(container);
		}



		template<typename DBAdapter, typename Result>
		void rangeM(std::string_view keyN,
							SearchTokenContainer		const	&tokenContainer,
							SearchTokenBufferKContainer		&tokenBKContainer,
							OutputBlob::Container			&container,
								uint32_t count, std::string_view keyStart,
										DBAdapter &db, Result &result){
			if (tokenContainer.size() == 1){
				// go back to single search

				auto const index = tokenContainer.front();

				return 	range1(keyN, index, container,
								count, keyStart,
									db, result);
			}

			using It  = typename DBAdapter::List::iterator;
			using FTS = shared::rsetmulti::fts::FTSIntersector<It, MaxSearchTokens>;

			FTS fts;

			for(auto const &index : tokenContainer){
				tokenBKContainer.push_back();

				auto const prefix = shared::rsetmulti::makeKeyDataSearch(tokenBKContainer.back(),
											DBAdapter::SEPARATOR,
												keyN, index);

				tokenBKContainer.push_back();
				auto const key = keyStart.empty() ? prefix :
							shared::rsetmulti::makeKeyDataStart(tokenBKContainer.back(),
											DBAdapter::SEPARATOR,
												keyN, index, keyStart);

				// auto const key = keyStart.empty() ? prefix : keyStart;

				logger<Logger::DEBUG>() << "MultiIndex2::rangeM" << "prefix" << prefix << "key" << key;

				using namespace shared::accumulate_results;

				StopPrefixPredicate stop{ prefix };

				bool const b = fts.push(
					prefix,
					DBAdapter::SEPARATOR[0],
					db->find(key),
					std::end(*db)
				);

				if (!b)
					return result.set_container0();
			}

			uint32_t results = 0;

			while(fts){
				auto const sv = fts();

				if (sv.empty()){
					container.push_back("");
					logger<Logger::DEBUG>() << "MultiIndex2::walk" << "input stream exhausted. break. iterations" << fts.getIterations();
					break;
				}

				if (++results > count){
					container.push_back(sv);

					logger<Logger::DEBUG>() << "MultiIndex2::walk" << "collected enough keys. break. iterations" << fts.getIterations();

					break;
				}

				container.push_back(FTS::fixItem(sv, DBAdapter::SEPARATOR[0]));
				container.push_back("1");

			//	logger<Logger::DEBUG>() << "MultiIndex2::walk" << "push" << sv;
			}

			return result.set_container(container);
		}

	} // namespace impl_



	template<class Protocol, class DBAdapter>
	struct IXMADD : BaseCommandRW<Protocol,DBAdapter>{

		IXMADD() : BaseCommandRW<Protocol,DBAdapter>("IXMADD", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// IXMADD keyN keySub keySort delimiter "words,words"

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 6)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_5);

			auto const keyN		= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			to_string_buffer_t bufferKeySort;

			auto const keySub	= p[2];
			// auto const keySort	= p[3]; // posponed
			auto const delimiter	= p[4];
			auto const tokens	= p[5];

			if (delimiter.size() != 1)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			if (keySub.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const keySort	= shared::sortkey::makeHashKeySort(keySub, p[3], bufferKeySort);

			if (!shared::rsetmulti::valid(keyN, keySub, keySort))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto &tokenContainer = blob.construct<OutputBlob::Container>();

			if (!validateTokensUser(delimiter[0], tokens, tokenContainer))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			// ---------------------

			auto &icontainer = blob.construct<OutputBlob::Container>();
			auto  bcontainer = std::nullptr_t{ nullptr }; // unused

			Decoder<DBAdapter> decoder;

			const hm4::Pair *pair = nullptr;

			constexpr std::string_view key = ""; // will be replaces later.

			IXMADDFactory factory{ key, pair,
							tokenContainer, icontainer, keySort };

			bool const withAutomaticKeySort = true;

			[[maybe_unused]]
			bool const b = shared::rsetmulti::add<withAutomaticKeySort>(db, decoder,
							keyN, keySub, keySort,
								icontainer, bcontainer,
									factory);

			return result.set_1();
		}



		struct IXMADDFactory : hm4::PairFactory::IFactoryAction<0,0,IXMADDFactory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<0,0,IXMADDFactory>;

			IXMADDFactory(std::string_view const key, const Pair *pair,
											OutputBlob::Container	&container,
											OutputBlob::Container	&icontainer,
											std::string_view	keySort) :
							Base::IFactoryAction	(key, impl_::sizeTokens(container, keySort), pair),
							container		(container	),
							icontainer		(icontainer	),
							keySort			(keySort	){}

			void action(Pair *pair){
				char   *raw = pair->getValC();
				size_t size = 0;

				for(size_t i = 0; i < container.size(); ++i){
					auto const sv = container[i];

					concatenateRawBuffer_(raw + size, sv);		size += sv.size();

					// if (i >= container.size() - 1)
					// 	continue;

					*(raw + size) = DBAdapter::SEPARATOR[0];	size += 1;
				}

				if constexpr(1){
				//	std::cout << ">>" << keySort << "<<\n";
					concatenateRawBuffer_(raw + size, keySort);	size += keySort.size();
				}

				// Decoder

				icontainer.clear(); // just in case
				icontainer.assign(std::begin(container), std::end(container));
			}

			auto const &getIndexes() const{
				return icontainer;
			}

		private:
			OutputBlob::Container	&container;
			OutputBlob::Container	&icontainer;
			std::string_view	keySort;
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ixmadd",	"IXMADD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IXMREM : BaseCommandRW<Protocol,DBAdapter>{

		IXMREM() : BaseCommandRW<Protocol,DBAdapter>("IXMREM", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// IXMREM keyN keySub...

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			auto const varg  = 2;

			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_2);

			auto const keyN		= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const keySub = *itk;

				if (keySub.empty())
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				// not 100% correct, because we do not have keySort
				if (!shared::rsetmulti::valid(keyN, keySub, shared::sortkey::keySortSize()))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}

			auto &icontainer = blob.construct<OutputBlob::Container>();
			auto  bcontainer = std::nullptr_t{ nullptr }; // unused

			Decoder<DBAdapter> decoder;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const keySub	= *itk;

				// to_string_buffer_t buffer;
				// auto const keySort	= shared::sortkey::makeHashKeySort(keySub, buffer);

				bool const withAutomaticKeySort = true;

				[[maybe_unused]]
				bool const b = shared::rsetmulti::rem<withAutomaticKeySort>(db, decoder,
								keyN, keySub, {},
									icontainer, bcontainer);
			}

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ixmrem",	"IXMREM"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IXMGETINDEXES : BaseCommandRO<Protocol,DBAdapter>{

		IXMGETINDEXES() : BaseCommandRO<Protocol,DBAdapter>("IXMGETINDEXES", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// MHGETINDEXES keyN keySub
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const keyN		= p[1];
			auto const keySub	= p[2];

			using namespace impl_;

			// not 100% correct, because we do not have keySort
			if (!shared::rsetmulti::valid(keyN, keySub, shared::sortkey::keySortSize()))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto &icontainer = blob.construct<OutputBlob::Container>();
			auto  bcontainer = std::nullptr_t{ nullptr }; // unused

			Decoder<DBAdapter> decoder;

			[[maybe_unused]]
			bool const b = shared::rsetmulti::getIndexes(db, decoder,
							keyN, keySub,
								icontainer, bcontainer);

			auto const &container = icontainer;

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ixmgetindexes",	"IXMGETINDEXES"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IXMSIM1 : BaseCommandRO<Protocol,DBAdapter>{

		IXMSIM1() : BaseCommandRO<Protocol,DBAdapter>("IXMSIM1", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// IXMSIM1 key word count from
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			auto const keyN     = p[1];
			auto const index    = p[2];

			if (keyN.empty() || index.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!shared::rsetmulti::valid(keyN, index))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace shared::config;

			auto const count    = myClamp<uint32_t>(p[3], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const keyStart = p[4];

			auto &container = blob.construct<OutputBlob::Container>();

			return range1(keyN, index,
							container,
								count, keyStart,
									db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ixmsim1",	"IXMSIM1"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IXMSIM : BaseCommandRO<Protocol,DBAdapter>{

		IXMSIM() : BaseCommandRO<Protocol,DBAdapter>("IXMSIM", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// IXMSIM key delimiter "words,words" count from
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 6)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_5);

			auto const keyN		= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const delimiter	= p[2];
			auto const tokens	= p[3];

			if (delimiter.size() != 1)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto &tokenContainer = blob.construct<SearchTokenContainer>();

			if (!validateTokensUser(delimiter[0], tokens, tokenContainer))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			// ---------------------

			using namespace shared::config;

			auto const count    = myClamp<uint32_t>(p[4], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const keyStart = p[5];

			auto &tokenBKContainer = blob.construct<SearchTokenBufferKContainer>();
			auto &container       = blob.construct<OutputBlob::Container>();

			return rangeM(keyN, tokenContainer, tokenBKContainer,
							container,
								count, keyStart,
									db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ixmsim",	"IXMSIM"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "mindex2";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				IXMADD		,
				IXMREM		,
				IXMGETINDEXES	,
				IXMSIM1		,
				IXMSIM
			>(pack);
		}
	};

} // namespace net::worker::commands::MultiIndex2

