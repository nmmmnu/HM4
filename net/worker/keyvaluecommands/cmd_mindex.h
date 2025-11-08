#include "base.h"

#include "shared_accumulateresults.h"

#include "stringtokenizer.h"
#include "pair_vfactory.h"
#include "ilist/txguard.h"

namespace net::worker::commands::MIndex{
	namespace mindex_impl_{
		using namespace net::worker::shared::accumulate_results;
		using namespace net::worker::shared::config;



		constexpr static bool valid(std::string_view keyN, std::string_view keySub, size_t more = 0){
			// keyN~word~sort~keySub, 3 * ~
			return hm4::Pair::isCompositeKeyValid(3 + more, keyN, keySub);
		}

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::string_view keySort, size_t more = 0){
			// keyN~word~sort~keySub, 3 * ~
			return hm4::Pair::isCompositeKeyValid(3 + more, keyN, keySub, keySort);
		}

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::string_view keySort, std::string_view txt, size_t more = 0){
			// keyN~word~sort~keySub, 3 * ~
			return hm4::Pair::isCompositeKeyValid(3 + more, keyN, keySub, keySort, txt);
		}

		std::string_view makeKeyCtrl(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub){

			return concatenateBuffer(bufferKey,
					keyN		,	separator	,
								separator	,
					keySub
			);
		}

		static std::string_view makeKeyData(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
						std::string_view txt,
						std::string_view sort
				){

			// keyN~word~sort~keySub

			return concatenateBuffer(bufferKey,
					keyN	,	separator	,
					txt	,	separator	,
					sort	,	separator	,
					keySub
			);
		}

		static std::string_view makeKeyDataSearch(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
						std::string_view txt
				){

			// keyN~word~

			return concatenateBuffer(bufferKey,
					keyN	,	separator	,
					txt	,	separator
			);
		}





		template<typename Container, typename CheckF>
		bool explodeTokens_(Container &container, std::string_view separator, std::string_view text, CheckF checkF){
			static_assert( std::is_constructible_v<typename Container::value_type, std::string_view> );

			StringTokenizer const tok{ text, separator[0] };

			size_t count = 0;

			for(auto const &x : tok){
				// we need to have space for the sort
				if (++count >= OutputBlob::ContainerSize)
					return false;

				if (!checkF(x))
					return false;

				container.push_back(x);
			}

			return true;
		}

		template<typename Container>
		bool sortTokens_(Container &container){
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

		template<typename Container, typename CheckF>
		bool prepareTokens(Container &container, std::string_view separator, std::string_view text, CheckF checkF){
			if (!explodeTokens_(container, separator, text, checkF))
				return false;

			return sortTokens_(container);
		}

		template<bool HasSortKey, size_t ContainerSize, typename Container>
		bool loadTokensSorted(Container &container, std::string_view separator, std::string_view text){
			static_assert( std::is_constructible_v<typename Container::value_type, std::string_view>);

			StringTokenizer const tok{ text, separator[0] };

			bool lastElement = false;

			for(auto const &x : tok){
				// sort key is inside
				if (container.size() > ContainerSize)
					return false;

				if (lastElement || x.empty())
					return false;

				// last element is sort and may not be sorted
				if (!container.empty() && container.back() >= x)
					lastElement = true;

				container.push_back(x);
			}

			if constexpr(HasSortKey){
				// inside must be: at least one token + sort key
				return container.size() >= 2;
			}else{
				return !container.empty();
			}
		}

		template<bool HasSortKey, size_t ContainerSize, typename Container>
		bool loadTokensUnsorted(Container &container, std::string_view separator, std::string_view text){
			static_assert( std::is_constructible_v<typename Container::value_type, std::string_view>);

			StringTokenizer const tok{ text, separator[0] };

			for(auto const &x : tok){
				if (container.size() > ContainerSize)
					return false;

				if (x.empty())
					continue;

				container.push_back(x);
			}

			// if the function returns false, this is cheap operation
			std::sort(std::begin(container), std::end(container));

			if constexpr(HasSortKey){
				// inside must be: at least one token + sort key
				return container.size() >= 2;
			}else{
				return !container.empty();
			}
		}

		template<typename Container>
		bool saveTokens(char *buffer, Container &container, std::string_view separator){
			// sort key is inside

			implodeRawBuffer_(buffer, container, separator);

			return true;
		}



		template<bool B, class DBAdapter>
		void mutate_(DBAdapter &db, std::string_view keyN, std::string_view keySub, std::string_view keySort, std::string_view text, std::string_view value, std::string_view msg){
			hm4::PairBufferKey bufferKey;

			auto const keyData = makeKeyData(bufferKey, DBAdapter::SEPARATOR,
						keyN, keySub,
							text,
								keySort
			);

			logger<Logger::DEBUG>() << msg << keyData;

			if constexpr(B){
				insert(*db, keyData, value);
			}else{
				erase(*db, keyData);
			}
		}

		template<class DBAdapter>
		void mutate(DBAdapter &db, std::string_view keyN, std::string_view keySub, std::string_view keySort, std::string_view text, std::string_view value, std::string_view msg){
			return mutate_<1>(db, keyN, keySub, keySort, text, value, msg);
		}

		template<class DBAdapter>
		void mutate(DBAdapter &db, std::string_view keyN, std::string_view keySub, std::string_view keySort, std::string_view text,                         std::string_view msg){
			return mutate_<0>(db, keyN, keySub, keySort, text, "", msg);
		}



		template<typename DBAdapter>
		std::string_view get(DBAdapter &db, std::string_view keyN, std::string_view keySub){
			hm4::PairBufferKey bufferKey;
			auto const keyCtrl = makeKeyCtrl(bufferKey, DBAdapter::SEPARATOR, keyN, keySub);

			logger<Logger::DEBUG>() << "MIndex::GET: ctrl key" << keyCtrl;

			if (auto const encodedValue = hm4::getPairVal(*db, keyCtrl); !encodedValue.empty()){
				// Case 1: ctrl key is set

				StringTokenizer const tok{ encodedValue, DBAdapter::SEPARATOR[0] };

				auto f = getForwardTokenizer(tok);
				auto l = getBackwardTokenizer(tok);

				auto const txt  = f();
				auto const txt2 = f(); // check that the list is long at least 2 words
				auto const sort = l();

				if (!txt.size() || !txt2.size() || !sort.size())
					return "";

				auto keyData = makeKeyData(bufferKey, DBAdapter::SEPARATOR,
						keyN,
						keySub,
							txt,
							sort
				);

				logger<Logger::DEBUG>() << "MIndex::GET: data key" << keyData;

				return hm4::getPairVal(*db, keyData);
			}

			return "";
		}



		inline std::string_view extractNth_(size_t const nth, char const separator, std::string_view const s){
			size_t count = 0;

			for (size_t i = 0; i < s.size(); ++i)
				if (s[i] == separator)
					if (++count; count == nth)
						return s.substr(i + 1);

			return "INVALID_DATA";
		}

		template<AccumulateOutput Out, class It, class Container>
		void accumulateResultsIXMOne(uint32_t const maxResults, std::string_view const prefix, It it, It eit, char const separator, Container &container){
			StopPrefixPredicate stop{ prefix };

			auto proj = [separator](std::string_view x){
				// keyN~word~
				return extractNth_(3, separator, x);
			};

			return accumulateResults<Out>(maxResults, stop, it, eit, container, proj);
		}

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
			namespace PN = mindex_impl_;

			if (p.size() != 7)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_5);

			auto const &keyN	= p[1];
			auto const &keySub	= p[2];
			auto const &delimiter	= p[3];
			auto const &tokens	= p[4];
			auto const &keySort	= p[5];
			auto const &value	= p[6];

			if (delimiter.size() != 1)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			if (!PN::valid(keyN, keySub, keySort))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);


			if (!hm4::Pair::isValValid(value))
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			auto &container = blob.construct<OutputBlob::Container>();

			auto checkF = [&](std::string_view txt){
				return PN::valid(keyN, keySub, keySort, txt);
			};

			if (!PN::prepareTokens(container, delimiter, tokens, checkF))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);



			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			using PN::mutate;

			// prepare old keys

			hm4::PairBufferKey bufferKey;
			auto const keyCtrl = PN::makeKeyCtrl(bufferKey, DBAdapter::SEPARATOR, keyN, keySub);

			if (auto const pair = hm4::getPairPtr(*db, keyCtrl); pair){
				// Case 1: ctrl key is set

				auto &oContainer = blob.construct<OutputBlob::Container>();
				auto &buferVal   = blob.allocate<hm4::PairBufferVal>();

				auto const valCtrl = concatenateBuffer(buferVal, pair->getVal());

				if (!PN::loadTokensSorted<1, OutputBlob::ContainerSize>(oContainer, DBAdapter::SEPARATOR, valCtrl)){
					// Case 1.0: invalid ctrl key, probable attack.

					logger<Logger::DEBUG>() << "MIndex::ADD: INVALID ctrl key" << keyCtrl;
				}else{
					// all OK, remove sort key from the container
					auto const oKeySort = oContainer.back();
					oContainer.pop_back();

					if (oKeySort != keySort){
						// remove all keys
						for(auto &x : oContainer)
							mutate(db, keyN, keySub, oKeySort, x, "MIndex::ADD: DEL index key");
					}else{
						// because keySort is the same,
						// we are skipping elements that will be inserted in a later

						#if 0

						// using binary search
						// O(N Log M)

						for(auto &x : oContainer)
							if (!std::binary_search(std::begin(container), std::end(container), x))
								mutate(db, keyN, keySub, oKeySort, x, "MIndex::ADD: optimized DEL index key");

						#else

						// using merge
						// O(N + M)

						auto itO = std::begin(oContainer);
						auto itN = std::begin(container);

						while (itO != std::end(oContainer) && itN != std::end(container)){
							int const comp = compare(itO->data(), itO->size(), itN->data(), itN->size());

							switch(comp){
							case -1:
								// inside old
								mutate(db, keyN, keySub, oKeySort, *itO, "MIndex::ADD: optimized DEL index key");
								++itO;
								break;

							case  0:
								++itO;
								++itN;
								break;

							default:
							case +1:
								++itN;
								break;
							}
						}

						// tail processing on the old...
						while(itO != std::end(oContainer)){
							mutate(db, keyN, keySub, keySort, *itO, "MIndex::ADD: TAIL DEL index key");
							++itO;
						}

						#endif
					}
				}

			}else{
				// Case 2: no ctrl key

				logger<Logger::DEBUG>() << "MIndex::ADD: no ctrl key" << keyCtrl;
			}

			// add new keys
			for(auto &x : container)
				mutate(db, keyN, keySub, keySort, x, value, "MIndex::ADD: SET index key");

			// add sort key, there will be space for it
			container.push_back(keySort);

			// set control key
			logger<Logger::DEBUG>() << "MIndex::ADD: SET ctrl key" << keyCtrl;

			hm4::Pair *hint = nullptr;
			hm4::insertV<IXMADD_Factory>(*db, keyCtrl, hint, container);

			return result.set(true);
		}

	private:
		struct IXMADD_Factory : hm4::PairFactory::IFactoryAction<0, 0, IXMADD_Factory>{
			using Pair      = hm4::Pair;
			using Base      = hm4::PairFactory::IFactoryAction<0, 0, IXMADD_Factory>;
			using Container = OutputBlob::Container;

			constexpr IXMADD_Factory(std::string_view const key, const Pair *pair, Container &container) :
							Base::IFactoryAction	(key, implodeBufferSize(container, SEPARATOR), pair),
							container			(container	){}

			void action(Pair *pair) const{
				namespace PN = mindex_impl_;

				char *data = pair->getValC();

				PN::saveTokens(data, container, SEPARATOR);
			}

		private:
			constexpr static auto SEPARATOR = DBAdapter::SEPARATOR;

		private:
			Container	&container;
		};

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
			namespace PN = mindex_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN   = p[1];
			auto const &keySub = p[2];

			if (!PN::valid(keyN, keySub))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			return result.set(
				PN::get(db, keyN, keySub)
			);
		}

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
			namespace PN = mindex_impl_;

			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_3);

			const auto &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (auto const &keySub = *itk; !PN::valid(keyN, keySub))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);



			auto &container = blob.construct<OutputBlob::Container>();

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &keySub = *itk;

				container.emplace_back(
					PN::get(db, keyN, keySub)
				);
			}

			return result.set_container(container);
		}

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

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			namespace PN = mindex_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN   = p[1];
			auto const &keySub = p[2];

			if (!PN::valid(keyN, keySub))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);



			hm4::PairBufferKey bufferKey;
			auto const keyCtrl = PN::makeKeyCtrl(bufferKey, DBAdapter::SEPARATOR, keyN, keySub);

			logger<Logger::DEBUG>() << "MIndex::EXISTS: ctrl key" << keyCtrl;

			return result.set(
				hm4::getPairOK(*db, keyCtrl)
			);
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
			namespace PN = mindex_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN   = p[1];
			auto const &keySub = p[2];

			if (!PN::valid(keyN, keySub))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);



			hm4::PairBufferKey bufferKey;
			auto const keyCtrl = PN::makeKeyCtrl(bufferKey, DBAdapter::SEPARATOR, keyN, keySub);

			if (auto const encodedValue = hm4::getPairVal(*db, keyCtrl); !encodedValue.empty()){
				auto &container = blob.construct<OutputBlob::Container>();

				if (PN::loadTokensSorted<1, OutputBlob::ContainerSize>(container, DBAdapter::SEPARATOR, encodedValue))
					return result.set_container(container);
			}

			std::array<std::string_view, 1> const container;
			return result.set_container(container);
		}

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
			namespace PN = mindex_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN   = p[1];
			auto const &keySub = p[2];

			if (!PN::valid(keyN, keySub))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);



			hm4::PairBufferKey bufferKey;
			auto const keyCtrl = PN::makeKeyCtrl(bufferKey, DBAdapter::SEPARATOR, keyN, keySub);

			if (auto const pair = hm4::getPairPtr(*db, keyCtrl); pair){
				// Case 1: ctrl key is set

				auto &container = blob.construct<OutputBlob::Container>();
				auto &buferVal  = blob.allocate<hm4::PairBufferVal>();

				auto const valCtrl = concatenateBuffer(buferVal, pair->getVal());

				[[maybe_unused]]
				hm4::TXGuard guard{ *db };

				// because we have a copy, we can remove it here.
				// HINT
				const auto *hint = pair;
				hm4::insertHintF<hm4::PairFactory::Tombstone>(*db, hint, keyCtrl);

				if (!PN::loadTokensSorted<1, OutputBlob::ContainerSize>(container, DBAdapter::SEPARATOR, valCtrl)){
					// Case 1.0: invalid ctrl key, probable attack.

					logger<Logger::DEBUG>() << "MIndex::REM: INVALID ctrl key" << keyCtrl;
				}else{
					// all OK, remove sort key from the container
					auto const keySort = container.back();
					container.pop_back();

					// remove all keys
					using PN::mutate;

					for(auto &x : container)
						mutate(db, keyN, keySub, keySort, x, "MIndex::REM: DEL index key");
				}

			}else{
				// Case 2: no ctrl key

				logger<Logger::DEBUG>() << "MIndex::REM: no ctrl key" << keyCtrl;
			}

			return result.set_1();
		}

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
			namespace PN = mindex_impl_;

			using namespace mindex_impl_;

			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			auto const keyN     = p[1];
			auto const index    = p[2];

			if (keyN.empty() || index.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const count    = myClamp<uint32_t>(p[3], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const keyStart = p[4];

			hm4::PairBufferKey bufferKey;
			auto const prefix = PN::makeKeyDataSearch(bufferKey, DBAdapter::SEPARATOR, keyN, index);

			auto const key = keyStart.empty() ? prefix : keyStart;

			logger<Logger::DEBUG>() << "IXMRANGE" << "prefix" << prefix;

			auto &container = blob.construct<OutputBlob::Container>();

			accumulateResultsIXMOne<AccumulateOutput::BOTH_WITH_TAIL>(
				count			,
				prefix			,
				db->find(key)		,
				std::end(*db)		,
				DBAdapter::SEPARATOR[0]	,
				container
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixmrange",	"IXMRANGE"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXMRANGEMULTI : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXMRANGEMULTI key delimiter "words,words" count from

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			namespace PN = mindex_impl_;

			using namespace mindex_impl_;

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

			auto &container = blob.construct<TokenContainer>();

			if (!PN::loadTokensUnsorted<0, MaxTokens>(container, delimiter, tokens))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			if (container.size() == 1){
				// optimized way for single word, same as IXMRANGE

				auto const &index = container[0];

				hm4::PairBufferKey bufferKey;
				auto const prefix = PN::makeKeyDataSearch(bufferKey, DBAdapter::SEPARATOR, keyN, index);

				auto const key = keyStart.empty() ? prefix : keyStart;

				logger<Logger::DEBUG>() << "IXMRANGEMILTI" << "prefix" << prefix;

				auto &container = blob.construct<OutputBlob::Container>();

				accumulateResultsIXMOne<AccumulateOutput::BOTH_WITH_TAIL>(
					count			,
					prefix			,
					db->find(key)		,
					std::end(*db)		,
					DBAdapter::SEPARATOR[0]	,
					container
				);

				return result.set_container(container);
			}else{
				return processMulti__(keyN, container, count, keyStart, db, result, blob);
			}
		}

	private:
		constexpr static size_t MaxTokens  = 32;

		using TokenContainer = StaticVector<std::string_view, MaxTokens>;

	private:
		static void processMulti__(std::string_view keyN, TokenContainer &tokens, uint32_t count, std::string_view keyStart,
					DBAdapter &db, Result<Protocol> &result, OutputBlob &blob);

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixmrangemulti",	"IXMRANGEMULTI"
		};
	};

	#include "cmd_mindex_ixmrangemulti.h.cc"



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "morton_curve_2d";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				IXMADD		,
				IXMGET		,
				IXMMGET		,
				IXMEXISTS	,
				IXMGETINDEXES	,
				IXMREM		,
				IXMRANGE	,
				IXMRANGEMULTI
			>(pack);
		}
	};




} // namespace


