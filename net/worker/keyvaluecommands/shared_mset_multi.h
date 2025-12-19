#ifndef SHARED_MSET_MULTI_H_
#define SHARED_MSET_MULTI_H_

#include "pair.h"
#include "stringtokenizer.h"
#include "ilist/txguard.h"

namespace net::worker::shared::msetmulti{

	using OutputBlob     = commands::OutputBlob;
	using ParamContainer = commands::ParamContainer;

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
				txt	,	separator
		);
	}

	inline std::string_view extractNth_(size_t const nth, char const separator, std::string_view const s){
		size_t count = 0;

		for (size_t i = 0; i < s.size(); ++i)
			if (s[i] == separator)
				if (++count; count == nth)
					return s.substr(i + 1);

		return "INVALID_DATA";
	}





	template<bool B, typename DBAdapter>
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

	template<typename DBAdapter>
	void mutate(DBAdapter &db, std::string_view keyN, std::string_view keySub, std::string_view keySort, std::string_view text, std::string_view value, std::string_view msg){
		return mutate_<1>(db, keyN, keySub, keySort, text, value, msg);
	}

	template<typename DBAdapter>
	void mutate(DBAdapter &db, std::string_view keyN, std::string_view keySub, std::string_view keySort, std::string_view text,                         std::string_view msg){
		return mutate_<0>(db, keyN, keySub, keySort, text, "", msg);
	}





	template<typename Decoder, typename Container, typename BufferVal, typename DBAdapter>
	bool add(DBAdapter &db,
			std::string_view keyN, std::string_view keySub, std::string_view tokens, char separator, std::string_view keySort, std::string_view value,
						Container &containerNew, Container &containerOld, BufferVal &buferVal){

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKeyCtrl(bufferKeyCtrl, DBAdapter::SEPARATOR, keyN, keySub);

		logger<Logger::DEBUG>() << "MSetMulti::ADD: ctrl key" << keyCtrl;

		[[maybe_unused]]
		hm4::TXGuard guard{ *db };

		Decoder decoder;

		if (!decoder.indexesUser(tokens, separator, containerNew))
			return false;

		// prepare old keys

		if (auto const pair = hm4::getPairPtr(*db, keyCtrl); pair){
			// Case 1: ctrl key is set

			auto const valCtrl = concatenateBuffer(buferVal, pair->getVal());

			if (!decoder.indexesStored(valCtrl, containerOld)){
				// Case 1.0: invalid ctrl key, probable attack.

				logger<Logger::DEBUG>() << "MSetMulti::ADD: INVALID ctrl key" << keyCtrl;
			}else{
				// all OK, remove sort key from the container
				auto const keySortOld = containerOld.back();
				containerOld.pop_back();

				if (keySortOld != keySort){
					// remove all keys
					for(auto &x : containerOld)
						if (valid(keyN, keySub, keySortOld, x)) // check consistency
							mutate(db, keyN, keySub, keySortOld, x, "MSetMulti::ADD: DEL index key");
						else
							logger<Logger::DEBUG>() << "MSetMulti::ADD: INVALID index key";
				}else{
					// because keySort is the same,
					// we are skipping elements that will be inserted in a later

					if constexpr(false){
						// using binary search
						// O(N Log M)

						for(auto &x : containerOld)
							if (!std::binary_search(std::begin(containerNew), std::end(containerNew), x))
								mutate(db, keyN, keySub, keySortOld, x, "MSetMulti::ADD: optimized DEL index key");
					}else{
						// using merge
						// O(N + M)

						auto itO = std::begin(containerOld);
						auto itN = std::begin(containerNew);

						while (itO != std::end(containerOld) && itN != std::end(containerNew)){
							int const comp = compare(itO->data(), itO->size(), itN->data(), itN->size());

							switch(comp){
							case -1:
								// inside old
								mutate(db, keyN, keySub, keySortOld, *itO, "MSetMulti::ADD: optimized DEL index key");
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
						while(itO != std::end(containerOld)){
							mutate(db, keyN, keySub, keySort, *itO, "MSetMulti::ADD: TAIL DEL index key");
							++itO;
						}

					} // end if
				}
			}

		}else{
			// Case 2: no ctrl key

			logger<Logger::DEBUG>() << "MSetMulti::ADD: no ctrl key" << keyCtrl;
		}

		// add new keys
		for(auto &x : containerNew)
			mutate(db, keyN, keySub, keySort, x, value, "MSetMulti::ADD: SET index key");

		// add sort key, there will be space for it
		containerNew.push_back(keySort);

		// set control key
		logger<Logger::DEBUG>() << "MSetMulti::ADD: SET ctrl key" << keyCtrl;

		struct ADD_Factory : hm4::PairFactory::IFactoryAction<0, 0, ADD_Factory>{
			using Pair	= hm4::Pair;
			using Base	= hm4::PairFactory::IFactoryAction<0, 0, ADD_Factory>;

			constexpr ADD_Factory(std::string_view const key, const Pair *pair,
								Container &container) :

							Base::IFactoryAction	(key, getSize__(container), pair),
							container			(container	){}

			void action(Pair *pair) const{
				constexpr auto SEPARATOR = DBAdapter::SEPARATOR;

				char *data = pair->getValC();

				implodeRawBuffer_(data, container, SEPARATOR);
			}

		private:
			static auto getSize__(Container const &container){
				constexpr auto SEPARATOR = DBAdapter::SEPARATOR;

				return implodeBufferSize(container, SEPARATOR);
			}

		private:
			Container	&container;
		};

		hm4::Pair *hint = nullptr;
		hm4::insertV<ADD_Factory>(*db, keyCtrl, hint, containerNew);

		return true;
	}



	template<typename Decoder, typename DBAdapter>
	std::string_view get(DBAdapter &db,
			std::string_view keyN, std::string_view keySub){

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKeyCtrl(bufferKeyCtrl, DBAdapter::SEPARATOR, keyN, keySub);

		logger<Logger::DEBUG>() << "MSetMulti::GET: ctrl key" << keyCtrl;

		Decoder decoder;

		if (auto const encodedValue = hm4::getPairVal(*db, keyCtrl); !encodedValue.empty()){
			// Case 1: ctrl key is set

			auto const [txt, sort] = decoder.value(encodedValue);

			if (valid(keyN, keySub, txt, sort)){
				hm4::PairBufferKey bufferKeyData;
				auto const keyData = makeKeyData(bufferKeyData, DBAdapter::SEPARATOR, keyN, keySub, txt, sort);

				logger<Logger::DEBUG>() << "MSetMulti::GET: data key" << keyData;

				return hm4::getPairVal(*db, keyData);
			}
		}

		return "";
	}



	template<typename Decoder, typename DBAdapter, typename Container>
	bool getIndexes(DBAdapter &db,
			std::string_view keyN, std::string_view keySub, Container &container){

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKeyCtrl(bufferKeyCtrl, DBAdapter::SEPARATOR, keyN, keySub);

		logger<Logger::DEBUG>() << "MSetMulti::GET_INDEX: ctrl key" << keyCtrl;

		Decoder decoder;

		if (auto const encodedValue = hm4::getPairVal(*db, keyCtrl); !encodedValue.empty())
			return decoder.indexesStored(encodedValue, container);

		return false;
	}



	template<typename DBAdapter>
	bool exists(DBAdapter &db,
			std::string_view keyN, std::string_view keySub){

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKeyCtrl(bufferKeyCtrl, DBAdapter::SEPARATOR, keyN, keySub);

		logger<Logger::DEBUG>() << "MSetMulti::EXISTS: ctrl key" << keyCtrl;

		return hm4::getPairOK(*db, keyCtrl);
	}



	template<typename DBAdapter, typename Result>
	void cmdProcessExists(ParamContainer const &p, DBAdapter &db, Result &result, OutputBlob &){
		// EXISTS key subkey0

		if (p.size() != 3)
			return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

		auto const &keyN   = p[1];
		auto const &keySub = p[2];

		if (keyN.empty() || keySub.empty())
			return result.set_error(ResultErrorMessages::EMPTY_KEY);

		if (!valid(keyN, keySub))
			return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

		return result.set(
			exists(db, keyN, keySub)
		);
	}



	template<typename Decoder, typename DBAdapter>
	void rem(DBAdapter &db,
			std::string_view keyN, std::string_view keySub, OutputBlob::Container &container, hm4::PairBufferVal &buferVal){

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKeyCtrl(bufferKeyCtrl, DBAdapter::SEPARATOR, keyN, keySub);

		logger<Logger::DEBUG>() << "MSetMulti::REM: ctrl key" << keyCtrl;

		Decoder decoder;

		if (auto const pair = hm4::getPairPtr(*db, keyCtrl); pair){
			// Case 1: ctrl key is set

			auto const encodedValue = concatenateBuffer(buferVal, pair->getVal());

			if (!decoder.indexesStored(encodedValue, container))
				return;

			auto const keySort = container.back();
			container.pop_back();

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			for(auto &x : container){
				if (valid(keyN, keySub, keySort, x)) // check consistency
					mutate(db, keyN, keySub, keySort, x, "MSetMulti::REM: DEL index key");
				else
					logger<Logger::DEBUG>() << "MSetMulti::REM: INVALID index key";
			}

			logger<Logger::DEBUG>() << "MSetMulti::REM: DEL ctrl key" << keyCtrl;

			erase(*db, keyCtrl);
		}
	}



	template<typename MyMDecoder, typename DBAdapter, typename Result>
	void cmdProcessRem(ParamContainer const &p, DBAdapter &db, Result &result, OutputBlob &blob){
		// REM key subkey0 subkey1 ...

		if (p.size() < 3)
			return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_3);

		const auto &keyN = p[1];

		if (keyN.empty())
			return result.set_error(ResultErrorMessages::EMPTY_KEY);

		auto const varg = 2;

		for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
			if (auto const &keySub = *itk; !valid(keyN, keySub))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
		}

		auto &container = blob.construct<OutputBlob::Container>();
		auto &buferVal  = blob.allocate<hm4::PairBufferVal>();

		for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
			auto const &keySub = *itk;

			container.clear();

			rem<MyMDecoder>(db, keyN, keySub, container, buferVal);
		}

		return result.set_1();
	}



	template<typename DBAdapter, typename Result>
	void cmdProcessRange(std::string_view keyN, std::string_view index, uint32_t count, std::string_view keyStart,
									DBAdapter &db, Result &result, OutputBlob &blob){

		using namespace net::worker::shared::accumulate_results;

		auto &container = blob.construct<OutputBlob::Container>();

		hm4::PairBufferKey bufferKey;
		auto const prefix = makeKeyDataSearch(bufferKey, DBAdapter::SEPARATOR, keyN, index);

		auto const key = keyStart.empty() ? prefix : keyStart;

		logger<Logger::DEBUG>() << "MSetMulti::cmdProcessRange" << "prefix" << prefix << "key" << key;

		StopPrefixPredicate stop{ prefix };

		auto proj = [](std::string_view x){
			auto const separator = DBAdapter::SEPARATOR[0];

			// keyN~word~
			return extractNth_(3, separator, x);
		};

		auto const Out = AccumulateOutput::BOTH_WITH_TAIL;

		sharedAccumulateResults<Out>(
			count		,
			stop		,
			db->find(key)	,
			std::end(*db)	,
			container	,
			proj
		);

		return result.set_container(container);
	}


	#include "shared_mset_multi_fts.h.cc"

} // namespace net::worker::shared::zsetmulti

#endif

