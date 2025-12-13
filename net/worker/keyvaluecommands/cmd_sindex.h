#include "base.h"

#include "shared_accumulateresults.h"

#include "stringtokenizer.h"
#include "utf8slidingwindow.h"
#include "pair_vfactory.h"
#include "ilist/txguard.h"

namespace net::worker::commands::SIndex{
	namespace sindex_impl_{
		using namespace net::worker::shared::accumulate_results;
		using namespace net::worker::shared::config;

		constexpr uint8_t NGram = 64;

		// + 1 for the separators
		static_assert(OutputBlob::ContainerSize * (NGram + 1) < hm4::PairConf::MAX_VAL_SIZE);



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

		inline std::string_view makeKeyData(hm4::PairBufferKey &bufferKey, std::string_view separator,
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

		inline std::string_view makeKeyDataSearch(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
						std::string_view txt
				){

			// keyN~word~

			return concatenateBuffer(bufferKey,
					keyN	,	separator	,
					txt	//	no separator here
			);
		}





		template<typename SlidingWindow, typename Container, typename CheckF>
		bool prepareTokensExplode_(Container &container, std::string_view text, CheckF checkF){
			static_assert( std::is_constructible_v<typename Container::value_type, std::string_view> );

			SlidingWindow sw{ text, NGram };

			for(auto const &x : sw){
				if (container.full())
					return false;

				if (!checkF(x))
					return false;

				container.push_back(x);
			}

			return true;
		}

		template<typename Container>
		bool prepareTokensSort_(Container &container){
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

		template<typename SlidingWindow, typename Container, typename CheckF>
		bool prepareTokens(Container &container, std::string_view text, CheckF checkF){
			if (!prepareTokensExplode_<SlidingWindow>(container, text, checkF))
				return false;

			return prepareTokensSort_(container);
		}

		template<typename Container>
		bool loadTokensStoredStored(Container &container, char separator, std::string_view text){
			static_assert( std::is_constructible_v<typename Container::value_type, std::string_view>);

			StringTokenizer const tok{ text, separator };

			bool lastElement = false;

			for(auto const &x : tok){
				// sort key is inside
				if (container.full())
					return false;

				if (lastElement || x.empty())
					return false;

				// last element is sort and may not be sorted
				if (!container.empty() && container.back() >= x)
					lastElement = true;

				container.push_back(x);
			}

			// inside must be: at least one token + sort key
			return container.size() >= 2;
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

	} // namespace sindex_impl_



	template<class Protocol, class DBAdapter>
	struct IXSADD : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXSADD a keySub "words,words" sort value

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using SlidingWindow = UTF8SlidingWindow;

			namespace PN = sindex_impl_;

			if (p.size() != 6)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_6);

			auto const &keyN	= p[1];
			auto const &keySub	= p[2];
			auto const &tokens	= p[3];
			auto const &keySort	= p[4];
			auto const &value	= p[5];

			if (!PN::valid(keyN, keySub, keySort))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			if (!hm4::Pair::isValValid(value))
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			auto &container = blob.construct<OutputBlob::Container>();

			auto checkF = [&](std::string_view s){
				return PN::valid(keyN, keySub, keySort, s);
			};

			if (!PN::prepareTokens<SlidingWindow>(container, tokens, checkF))
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

				if (!PN::loadTokensStoredStored(oContainer, DBAdapter::SEPARATOR[0], valCtrl)){
					// Case 1.0: invalid ctrl key, probable attack.

					logger<Logger::DEBUG>() << "MIndex::ADD: INVALID ctrl key" << keyCtrl;
				}else{
					// all OK, remove sort key from the container
					auto const oKeySort = oContainer.back();
					oContainer.pop_back();

					if (oKeySort != keySort){
						// remove all keys
						for(auto &x : oContainer)
							if (PN::valid(keyN, keySub, oKeySort, x)) // check consistency
								mutate(db, keyN, keySub, oKeySort, x, "MIndex::ADD: DEL index key");
							else
								logger<Logger::DEBUG>() << "MIndex::ADD: INVALID index key";
					}else{
						// because keySort is the same,
						// we are skipping elements that will be inserted in a later

						if constexpr(false){
							// using binary search
							// O(N Log M)

							for(auto &x : oContainer)
								if (!std::binary_search(std::begin(container), std::end(container), x))
									mutate(db, keyN, keySub, oKeySort, x, "MIndex::ADD: optimized DEL index key");
						}else{
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

						} // end if
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
			hm4::insertV<IXSADD_Factory>(*db, keyCtrl, hint, container);

			return result.set(true);
		}

	private:
		struct IXSADD_Factory : hm4::PairFactory::IFactoryAction<0, 0, IXSADD_Factory>{
			using Pair	= hm4::Pair;
			using Base	= hm4::PairFactory::IFactoryAction<0, 0, IXSADD_Factory>;
			using Container	= OutputBlob::Container;

			constexpr IXSADD_Factory(std::string_view const key, const Pair *pair,
								Container &container) :

							Base::IFactoryAction	(key, getSize__(container), pair),
							container			(container	){}

			void action(Pair *pair) const{
				namespace PN = sindex_impl_;

				char *data = pair->getValC();

				implodeRawBuffer_(data, container, SEPARATOR);
			}

		private:
			constexpr static auto SEPARATOR = DBAdapter::SEPARATOR;

			static auto getSize__(Container const &container){
				return implodeBufferSize(container, SEPARATOR);
			}

		private:
			Container	&container;
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixsadd",	"IXSADD"
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
			namespace PN = sindex_impl_;

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
			"ixsget",	"IXSGET"
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
			namespace PN = sindex_impl_;

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
			"ixsmget",	"IXSMGET"
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

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			namespace PN = sindex_impl_;

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
			"ixsexists",	"IXSEXISTS"
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
			namespace PN = sindex_impl_;

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

				if (PN::loadTokensStoredStored(container, DBAdapter::SEPARATOR[0], encodedValue)){
					auto const keySort = container.back();

					// container is guaranteed to be at least 2 elements

					for(auto it = std::begin(container); it != std::prev(std::end(container)); ++it)
						if (!PN::valid(keyN, keySub, keySort, *it)) // check consistency
							*it = "INVALID_DATA";

					return result.set_container(container); // we do not checking, because is harmless
				}
			}

			std::array<std::string_view, 1> const container;
			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixsgetindexes",	"IXSGETINDEXES"
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
			namespace PN = sindex_impl_;

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

				if (!PN::loadTokensStoredStored(container, DBAdapter::SEPARATOR[0], valCtrl)){
					// Case 1.0: invalid ctrl key, probable attack.

					logger<Logger::DEBUG>() << "MIndex::REM: INVALID ctrl key" << keyCtrl;
				}else{
					// all OK, remove sort key from the container
					auto const keySort = container.back();
					container.pop_back();

					// remove all keys
					using PN::mutate;

					for(auto &x : container)
						if (PN::valid(keyN, keySub, keySort, x)) // check consistency
							mutate(db, keyN, keySub, keySort, x, "MIndex::REM: DEL index key");
						else
							logger<Logger::DEBUG>() << "MIndex::REM: INVALID index key";
				}

			}else{
				// Case 2: no ctrl key

				logger<Logger::DEBUG>() << "MIndex::REM: no ctrl key" << keyCtrl;
			}

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixsrem",	"IXSREM",
			"ixsremove",	"IXSREMOVE",
			"ixsdel",	"IXSDEL"
		};
	};


	template<class Protocol, class DBAdapter>
	struct IXSRANGE : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXSRANGE key txt count from

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			namespace PN = sindex_impl_;

			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			auto const keyN     = p[1];
			auto const index    = p[2];

			if (keyN.empty() || index.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!PN::valid(keyN, index))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const count    = myClamp<uint32_t>(p[3], PN::ITERATIONS_RESULTS_MIN, PN::ITERATIONS_RESULTS_MAX);
			auto const keyStart = p[4];



			auto &ucontainer = blob.construct<UContainer>();



			hm4::PairBufferKey bufferKey;
			auto const prefix = PN::makeKeyDataSearch(bufferKey, DBAdapter::SEPARATOR, keyN, index);

			auto const key = keyStart.empty() ? prefix : keyStart;

			logger<Logger::DEBUG>() << "IXSRANGE*" << "prefix" << prefix << "key" << key;

			PN::StopPrefixPredicate stop{ prefix };

			std::string_view tail;

			auto pTail = [&](std::string_view const key = "") mutable{
				tail = key;
			};

			auto pPair = [&](auto const &pair) mutable -> bool{
				auto const separator = DBAdapter::SEPARATOR[0];

				// key~word~sort~keyN
				auto const key  = PN::extractNth_(3, separator, pair.getKey());
				auto const sort = PN::extractNth_(2, separator, pair.getKey());
				auto const val  = pair.getVal();

				ucontainer.emplace_back(key, sort, val);

				return true;
			};

			PN::sharedAccumulatePairs(
				count		,
				stop		,
				db->find(key)	,
				std::end(*db)	,
				pTail		,
				pPair
			);

			std::sort(std::begin(ucontainer), std::end(ucontainer));

			auto uit = std::unique(std::begin(ucontainer), std::end(ucontainer));

			auto &container = blob.construct<OutputBlob::Container>();

			for(auto it = std::begin(ucontainer); it != uit; ++it){
				container.push_back(it->key);
				container.push_back(it->val);
			}

			container.push_back(tail);

			return result.set_container(container);
		}

	private:
		struct UItem{
			std::string_view key;
			std::string_view sort;
			std::string_view val;

			constexpr UItem(std::string_view key, std::string_view sort, std::string_view val) :
							key	(key	),
							sort	(sort	),
							val	(val	){}

			constexpr bool operator  <(UItem const &other) const{
				// same keys are with same sort
				if (auto const comp = sort.compare(other.sort); comp)
					return comp < 0;

				return key < other.key;
			}

			constexpr bool operator ==(UItem const &other) const{
				return key == other.key;
			}
		};

		using UContainer = StaticVector<UItem, OutputBlob::ContainerSize>;

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixsrange",	"IXSRANGE"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "sindex";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				IXSADD		,
				IXSGET		,
				IXSMGET		,
				IXSEXISTS	,
				IXSGETINDEXES	,
				IXSREM		,
				IXSRANGE
			>(pack);
		}
	};




} // namespace


