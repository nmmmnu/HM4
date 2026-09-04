#ifndef SHARED_RSET_MULTI_H_
#define SHARED_RSET_MULTI_H_

#include "pair.h"
#include "ilist/txguard.h"

/*
Reverse Set Multi

- One to Many set
- Automatic keySort
- Uses Decoder

keyN~~keySub			-> encoded data

keyN~INDEX0~keySort~keySub	-> keySub
keyN~INDEX1~keySort~keySub	-> keySub
keyN~INDEX2~keySort~keySub	-> keySub
*/

namespace net::worker::shared::rsetmulti{

	constexpr static bool valid(std::string_view keyN, std::string_view keySub, size_t more = 0){
		// keyN~word~keySort~keySub, 3 * ~
		return hm4::Pair::isCompositeKeyValid(3 + more, keyN, keySub);
	}

	constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::string_view keySort, size_t more = 0){
		// keyN~word~keySort~keySub, 3 * ~
		return hm4::Pair::isCompositeKeyValid(3 + more, keyN, keySub, keySort);
	}

	constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::string_view keySort, std::string_view text, size_t more = 0){
		// keyN~word~keySort~keySub, 3 * ~
		return hm4::Pair::isCompositeKeyValid(3 + more, keyN, keySub, keySort, text);
	}

	inline std::string_view makeKeyCtrl(hm4::PairBufferKey &bufferKey, std::string_view separator,
				std::string_view keyN,
				std::string_view keySub){

		// keyN~~keySub

		return concatenateBuffer(bufferKey,
				keyN		,	separator	,
							separator	,
				keySub
		);
	}

	inline std::string_view makeKeyValue(hm4::PairBufferKey &bufferKey, std::string_view separator,
				std::string_view keyN,
				std::string_view keySub){

		// keyN~~keySub~

		return concatenateBuffer(bufferKey,
				keyN		,	separator	,
							separator	,
				keySub		,	separator
		);
	}

	inline std::string_view makeKeyData(hm4::PairBufferKey &bufferKey, std::string_view separator,
				std::string_view keyN,
				std::string_view keySub,
				std::string_view keySort,
					std::string_view text
			){

		// keyN~text~keySub

		return concatenateBuffer(bufferKey,
				keyN	,	separator	,
				text	,	separator	,
				keySort	,	separator	,
				keySub
		);
	}

	inline std::string_view makeKeyDataSearch(hm4::PairBufferKey &bufferKey, std::string_view separator,
				std::string_view keyN,
					std::string_view text
			){

		// keyN~text~

		return concatenateBuffer(bufferKey,
				keyN	,	separator	,
				text	,	separator
		);
	}

	inline std::string_view makeKeyDataStart(hm4::PairBufferKey &bufferKey, std::string_view separator,
				std::string_view keyN,
					std::string_view text,
						std::string_view keyStart
			){

		// keyN~text~SORT+KEYSUB

		return concatenateBuffer(bufferKey,
				keyN	,	separator	,
				text	,	separator	,
				keyStart
		);
	}

	inline std::string_view makeKeyDataSearchNS(hm4::PairBufferKey &bufferKey, std::string_view separator,
				std::string_view keyN,
					std::string_view text
			){

		// keyN~text~

		return concatenateBuffer(bufferKey,
				keyN	,	separator	,
				text
		);
	}

	inline std::string_view makeKeyDataSearchFlat(hm4::PairBufferKey &bufferKey, std::string_view separator,
				std::string_view keyN
			){

		// keyN~~

		return concatenateBuffer(bufferKey,
				keyN		,	separator	,
							separator
		);
	}



	namespace impl_{

		namespace mut{

			template<bool Insert, typename DBAdapter>
			void mutate_(DBAdapter &db,
					std::string_view keyN, std::string_view keySub, std::string_view keySort, std::string_view text,
						std::string_view value, std::string_view msg){

				hm4::PairBufferKey bufferKey;

				auto const keyData = makeKeyData(bufferKey, DBAdapter::SEPARATOR,
							keyN, keySub, keySort,
								text
				);

				logger<Logger::DEBUG>() << msg << keyData;

				if constexpr(Insert){
					insert(*db, keyData, value);
				}else{
					erase(*db, keyData);
				}
			}

			template<typename DBAdapter>
			void mutate(DBAdapter &db,
					std::string_view keyN, std::string_view keySub, std::string_view keySort, std::string_view text,
						std::string_view value, std::string_view msg){

				return mutate_<1>(db, keyN, keySub, keySort, text, value, msg);
			}

			template<typename DBAdapter>
			void mutate(DBAdapter &db,
					std::string_view keyN, std::string_view keySub, std::string_view keySort, std::string_view text,
						std::string_view msg){

				return mutate_<0>(db, keyN, keySub, keySort, text, "",    msg);
			}

		} // namespace mut

		template<typename DBAdapter, typename Container>
		void removeKeys(DBAdapter &db, Container const &icontainer,
					std::string_view keyN, std::string_view keySub, std::string_view keySort){

			using namespace mut;

			for(auto const &txt : icontainer){
				if (valid(keyN, keySub, keySort, txt)){ // check consistency
					mutate(db, keyN, keySub, keySort, txt, "MSetMulti::ADD/REM: del index key");
				}else{
					logger<Logger::DEBUG>() << "MSetMulti::ADD/REM: invalid index key";
				}
			}
		}

		template<typename DBAdapter, typename Container>
		void insertKeys(DBAdapter &db, Container const &icontainer,
					std::string_view keyN, std::string_view keySub, std::string_view keySort){

			using namespace mut;

			auto const value = keySub;

			for(auto const &txt : icontainer){
				if (valid(keyN, keySub, keySort, txt)) // check consistency
					mutate(db, keyN, keySub, keySort, txt, value, "MSetMulti::ADD: set index key");
				else
					logger<Logger::DEBUG>() << "MSetMulti::ADD: invalid set index key";
			}
		}

		template<bool withAutomaticKeySort = false, typename DBAdapter, typename Decoder, typename Container, typename BContainer>
		bool getIndexes(DBAdapter &db, Decoder decoder,
					std::string_view keyCtrl,
						Container &icontainer, BContainer &bcontainer){

			auto const pair = [&](){
				if (auto const bytes = decoder.bytes(); bytes){
			//	if constexpr(auto const bytes = Decoder::bytes(); bytes){
					return hm4::getPairPtrWithSize(*db, keyCtrl, bytes);
				}else{
					return hm4::getPairPtr(*db, keyCtrl);
				}
			}();

			if (!pair)
				return false;

			if constexpr(withAutomaticKeySort){
				return decoder(std::true_type{}, pair->getVal(), icontainer, bcontainer);
			}else{
				return decoder(pair->getVal(), icontainer, bcontainer);
			}
		}

	} // namespace impl_



	template<bool withAutomaticKeySort = false, typename DBAdapter, typename Decoder, typename Container, typename BContainer, typename Factory>
	bool add(DBAdapter &db, Decoder decoder,
			std::string_view keyN, std::string_view keySub, std::string_view keySort,
						Container &icontainer, BContainer &bcontainer,
							Factory &factory){

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKeyCtrl(bufferKeyCtrl,   DBAdapter::SEPARATOR, keyN, keySub);

		logger<Logger::DEBUG>() << "MSetMulti::GET_INDEXES: ctrl key" << keyCtrl;

		// get indexes
		// if this returns false then this means, key is new.
		bool const old = impl_::getIndexes<withAutomaticKeySort>(db, decoder, keyCtrl, icontainer, bcontainer);

		[[maybe_unused]]
		hm4::TXGuard guard{ *db };

		// remove all old keys,
		// because we do not know the new keys yet, we have to delete all
		if (old){
			if constexpr(withAutomaticKeySort){
				// use keySort in the array
				auto const keySort = icontainer.back();
				icontainer.pop_back();

				impl_::removeKeys(db, icontainer, keyN, keySub, keySort);
			}else{
				// use keySort from arguments
				impl_::removeKeys(db, icontainer, keyN, keySub, keySort);
			}
		}

		// icontainer, bcontainer no longer used.

		factory.setKey(keyCtrl);
		hm4::insertVF(*db, factory);

		// indexes are in the factory

		// insert all keys
		impl_::insertKeys(db, factory.getIndexes(), keyN, keySub, keySort);

		return true;
	}

	template<bool withAutomaticKeySort = false, typename DBAdapter, typename Decoder, typename Container, typename BContainer>
	bool rem(DBAdapter &db, Decoder decoder,
				std::string_view keyN, std::string_view keySub, std::string_view keySort,
					Container &icontainer, BContainer &bcontainer){

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKeyCtrl(bufferKeyCtrl,   DBAdapter::SEPARATOR, keyN, keySub);

		logger<Logger::DEBUG>() << "MSetMulti::REM: ctrl key" << keyCtrl;

		if (!impl_::getIndexes<withAutomaticKeySort>(db, decoder, keyCtrl, icontainer, bcontainer))
			return false;

		[[maybe_unused]]
		hm4::TXGuard guard{ *db };

		if constexpr(withAutomaticKeySort){
			// use keySort in the array
			auto const keySort = icontainer.back();
			icontainer.pop_back();

			impl_::removeKeys(db, icontainer, keyN, keySub, keySort);
		}else{
			// use keySort from arguments
			impl_::removeKeys(db, icontainer, keyN, keySub, keySort);
		}

		erase(*db, keyCtrl);

		return true;
	}

	template<typename DBAdapter, typename Decoder, typename Container, typename BContainer>
	bool getIndexes(DBAdapter &db, Decoder decoder,
				std::string_view keyN, std::string_view keySub,
					Container &icontainer, BContainer &bcontainer){

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKeyCtrl(bufferKeyCtrl,   DBAdapter::SEPARATOR, keyN, keySub);

		return impl_::getIndexes(db, decoder, keyCtrl, icontainer, bcontainer);
	}

	template<typename DBAdapter>
	std::string_view getData(DBAdapter &db,
				std::string_view keyN, std::string_view keySub){

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKeyCtrl(bufferKeyCtrl,   DBAdapter::SEPARATOR, keyN, keySub);

		return hm4::getPairVal(*db, keyCtrl);
	}

} // namespace net::worker::shared::msetmulti_better

#endif







/*
		// update all keys, using merge
		// O(N + M)

		auto itO = std::begin(container0);
		auto itN = std::begin(container1);

		while (itO != std::end(container0) && itN != std::end(container1)){
			int const comp = compare(
						itO->data(), itO->size(),
						itN->data(), itN->size()
			);

			switch(comp){
			case -1:
				// delete old
				mutate(db, keyN, keySub, *itO,        "MSetMulti::ADD: optimized DEL index key");
				++itO;
				break;

			case  0:
				// value is the same, preserve
				++itO;
				++itN;
				break;

			default:
			case +1:
				// insert new
				mutate(db, keyN, keySub, *itN, value, "MSetMulti::ADD: optimized ADD index key");
				++itN;
				break;
			}
		}

		// tail processing on the old...
		while(itO != std::end(container0)){
				mutate(db, keyN, keySub, *itO,        "MSetMulti::ADD: optimized DEL index key");
				++itO;
		}

		auto const value = keyN;

		// tail processing on the new...
		while(itN != std::end(container1)){
				mutate(db, keyN, keySub, *itN, value, "MSetMulti::ADD: optimized ADD index key");
				++itN;
		}
*/


