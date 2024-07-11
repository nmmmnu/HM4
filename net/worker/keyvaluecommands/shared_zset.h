#ifndef SHARED_ZSET_H_
#define SHARED_ZSET_H_

#include "mystring.h"

namespace net::worker::shared::zset{

	constexpr bool isKeyValid(std::string_view keyN, std::string_view keySub, size_t score_size){
		return hm4::Pair::isCompositeKeyValid(1 + 1 + score_size, keyN, keySub);
	}

	constexpr bool isKeyValid(std::string_view keyN, size_t score_size){
		return hm4::Pair::isCompositeKeyValid(1 + 1 + score_size, keyN);
	}

	constexpr bool isKeyValid(std::string_view keyN, std::string_view keySub, std::string_view score){
		return hm4::Pair::isCompositeKeyValid(1 + 1, keyN, keySub, score);
	}

	constexpr bool isKeyValid(std::string_view keyN, std::string_view score){
		return hm4::Pair::isCompositeKeyValid(1 + 1, keyN,         score);
	}



	namespace impl_{
		std::string_view makeKey(std::string_view separator,
				std::string_view keyN, std::string_view keySub, std::string_view score,
				hm4::PairBufferKey &bufferKey){

			return concatenateBuffer(bufferKey,
						keyN		,
						separator	,
						score		,
						separator	,
						keySub
			);
		}

		std::string_view makeKey(std::string_view separator,
				std::string_view keyN, std::string_view keySub,
				hm4::PairBufferKey &bufferKey){

			return concatenateBuffer(bufferKey,
						keyN		,
						separator	,
						separator	,
						keySub
			);
		}



		struct StandardScoreController{
			constexpr static bool canGetValue = false;

			constexpr static std::string_view decode(std::string_view score){
				return score;
			}

			constexpr static std::string_view encode(std::string_view score, std::string_view){
				return score;
			}
		};



		template<typename SC, typename DBAdapter>
		std::string_view getScoreFromControlKey(DBAdapter &db, std::string_view keyCtrl){
			return SC::decode( hm4::getPairVal(*db, keyCtrl) );
		}

		template<typename SC, typename DBAdapter>
		std::string_view getValueFromControlKey(DBAdapter &db, std::string_view keyCtrl){
			static_assert(SC::canGetValue);

			return SC::getValue( hm4::getPairVal(*db, keyCtrl) );
		}

	} // namespace impl_



	template<typename SC = impl_::StandardScoreController, typename DBAdapter>
	void add(DBAdapter &db,
			std::string_view keyN, std::string_view keySub, std::string_view score, std::string_view value){

		using namespace impl_;

		{
			// Update control key and delete hash key if any

			hm4::PairBufferKey bufferKeyCtrl;
			auto const keyCtrl = makeKey(DBAdapter::SEPARATOR, keyN, keySub, bufferKeyCtrl);

			logger<Logger::DEBUG>() << "ZSet GET ctrl key" << keyCtrl;

			if (const auto *pair = hm4::getPairPtr(*db, keyCtrl); pair){
				// Case 1: ctrl key is set

				if (auto const scoreOld = SC::decode(pair->getVal()); !isKeyValid(keyN, keySub, scoreOld)){
					// Case 1.0: invalid ctrl key, probable attack.

					logger<Logger::DEBUG>() << "ZSet INVALID SET ctrl key" << keyCtrl;

					// HINT
					const auto *hint = pair;
					hm4::insertHintF<hm4::PairFactory::Normal>(*db, hint, keyCtrl, SC::encode(score, value));
				}else if (scoreOld == score){
					// Case 1.1: ctrl key is same, no need to be updated.

					logger<Logger::DEBUG>() << "ZSet SKIP SET ctrl key";
				}else{
					// Case 1.2: old ctrl key has to be updated

					hm4::PairBufferKey bufferKeyHashOld;
					auto const keyHashOld = makeKey(DBAdapter::SEPARATOR, keyN, keySub, scoreOld, bufferKeyHashOld);

					logger<Logger::DEBUG>() << "ZSet SET ctrl key" << keyCtrl;

					// HINT
					const auto *hint = pair;
					hm4::insertHintF<hm4::PairFactory::Normal>(*db, hint, keyCtrl, SC::encode(score, value));

					logger<Logger::DEBUG>() << "ZSet DEL old hash key" << keyHashOld;

					erase(*db, keyHashOld);
				}
			}else{
				// Case 2: no ctrl key

				logger<Logger::DEBUG>() << "ZSet SET ctrl key" << keyCtrl;

				insert(*db, keyCtrl, SC::encode(score, value));
			}
		}

		hm4::PairBufferKey bufferKeyHash;
		auto const keyHash = makeKey(DBAdapter::SEPARATOR, keyN, keySub, score, bufferKeyHash);

		logger<Logger::DEBUG>() << "ZSet SET hash key" << keyHash;

		insert(*db, keyHash, value);
	}

	template<typename SC = impl_::StandardScoreController, typename DBAdapter>
	void rem(DBAdapter &db,
			std::string_view keyN, std::string_view keySub){

		using namespace impl_;

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKey(DBAdapter::SEPARATOR, keyN, keySub, bufferKeyCtrl);

		logger<Logger::DEBUG>() << "ZSet GET ctrl key" << keyCtrl;

		if (const auto *pair = hm4::getPairPtr(*db, keyCtrl); pair){
			// Case 1: ctrl key is set

			if (auto const score = SC::decode(pair->getVal()); !isKeyValid(keyN, keySub, score)){
				// Case 1.0: invalid ctrl key, probable attack.

				logger<Logger::DEBUG>() << "ZSet INVALID SET ctrl key" << keyCtrl;

				// HINT
				const auto *hint = pair;
				hm4::insertHintF<hm4::PairFactory::Tombstone>(*db, hint, keyCtrl);
			}else{
				// Case 1.1: ctrl key has to be removed

				hm4::PairBufferKey bufferKeyHash;
				auto const keyHash = makeKey(DBAdapter::SEPARATOR, keyN, keySub, score, bufferKeyHash);

				logger<Logger::DEBUG>() << "ZSet DEL ctrl key" << keyCtrl;

				// HINT
				const auto *hint = pair;
				hm4::insertHintF<hm4::PairFactory::Tombstone>(*db, hint, keyCtrl);

				logger<Logger::DEBUG>() << "ZSet DEL hash key" << keyHash;

				erase(*db, keyHash);
			}
		}else{
			// Case 2: no ctrl key

			logger<Logger::DEBUG>() << "ZSet no ctrl key";
		}
	}

	template<typename SC = impl_::StandardScoreController, typename DBAdapter>
	std::string_view get(DBAdapter &db,
			std::string_view keyN, std::string_view keySub){

		using namespace impl_;

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKey(DBAdapter::SEPARATOR, keyN, keySub, bufferKeyCtrl);

		logger<Logger::DEBUG>() << "ZSet GET ctrl key" << keyCtrl;

		if constexpr(SC::canGetValue){
			// Shortcut procedure

			if (auto const value = getValueFromControlKey<SC>(db, keyCtrl); !value.empty()){
				// Case 1: ctrl key is set
				return value;
			}else{
				// Case 2: no ctrl key
				logger<Logger::DEBUG>() << "ZSet INVALID ctrl key" << keyCtrl;

				return "";
			}
		}else{
			// standard procedure

			if (auto const score = getScoreFromControlKey<SC>(db, keyCtrl); !score.empty()){
				// Case 1: ctrl key is set

				if (!isKeyValid(keyN, keySub, score)){
					// Case 1.0: invalid ctrl key, probable attack.
					logger<Logger::DEBUG>() << "ZSet INVALID ctrl key" << keyCtrl;

					return "";
				}

				// Case 1.1: valid ctrl key.

				hm4::PairBufferKey bufferKeyHash;
				auto const keyHash = makeKey(DBAdapter::SEPARATOR, keyN, keySub, score, bufferKeyHash);

				logger<Logger::DEBUG>() << "ZSet GET hash key" << keyHash;

				return hm4::getPairVal(*db, keyHash);
			}else{
				// Case 2: no ctrl key

				logger<Logger::DEBUG>() << "ZSet no ctrl key";

				return "";
			}
		} // if constexpr
	}

	template<typename SC = impl_::StandardScoreController, typename DBAdapter>
	bool exists(DBAdapter &db,
			std::string_view keyN, std::string_view keySub){

		using namespace impl_;

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKey(DBAdapter::SEPARATOR, keyN, keySub, bufferKeyCtrl);

		logger<Logger::DEBUG>() << "ZSet GET ctrl key" << keyCtrl;

		if (auto const score = getScoreFromControlKey<SC>(db, keyCtrl); !score.empty()){
			// Case 1: ctrl key is set

			if (!isKeyValid(keyN, keySub, score)){
				// Case 1.0: invalid ctrl key, probable attack.
				logger<Logger::DEBUG>() << "ZSet INVALID ctrl key" << keyCtrl;

				return false;
			}

			// Case 1.1: valid ctrl key.

			return true;
		}else{
			// Case 2: no ctrl key

			logger<Logger::DEBUG>() << "ZSet no ctrl key";

			return false;
		}
	}

	constexpr std::string_view extractScore(std::string_view key, size_t keyNSize, size_t scoreSize){
		if (key.size() > keyNSize)
			return key.substr(keyNSize + 1, scoreSize);
		else
			return key;
	};

	constexpr std::string_view extractScore(std::string_view key, std::string_view keyN, size_t scoreSize){
		return extractScore(key, keyN.size(), scoreSize);
	};

	constexpr std::string_view extractKeySub(std::string_view key, size_t keyNSize, size_t scoreSize){
		if (key.size() > keyNSize)
			return key.substr(keyNSize + 1 + scoreSize + 1);
		else
			return key;
	};

	constexpr std::string_view extractKeySub(std::string_view key, std::string_view keyN, size_t scoreSize){
		return extractKeySub(key, keyN.size(), scoreSize);
	};

	template<typename SC = impl_::StandardScoreController, typename DBAdapter>
	std::string_view score(DBAdapter &db,
			std::string_view keyN, std::string_view keySub){

		using namespace impl_;

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKey(DBAdapter::SEPARATOR, keyN, keySub, bufferKeyCtrl);

		logger<Logger::DEBUG>() << "ZScore GET ctrl key" << keyCtrl;

		if (auto const score = getScoreFromControlKey<SC>(db, keyCtrl); !score.empty()){
			// Case 1: ctrl key is set

			if (!isKeyValid(keyN, keySub, score)){
				// Case 1.0: invalid ctrl key, probable attack.
				logger<Logger::DEBUG>() << "ZSet INVALID ctrl key" << keyCtrl;

				return "";
			}

			// Case 1.1: valid ctrl key.
			return score;
		}else{
			// Case 2: no ctrl key
			return "";
		}
	}



	template<typename ParamContainer, typename OutputBlob, typename Result, typename DBAdapter>
	void cmdProcessExists(ParamContainer const &p, DBAdapter &db, Result &result, OutputBlob &, size_t scoreSize){
		// EXISTS key subkey0

		if (p.size() != 3)
			return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

		auto const &keyN   = p[1];
		auto const &keySub = p[2];

		if (keyN.empty() || keySub.empty())
			return result.set_error(ResultErrorMessages::EMPTY_KEY);

		if (!isKeyValid(keyN, keySub, scoreSize))
			return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

		return result.set(
			exists(db, keyN, keySub)
		);
	}

	template<typename SC = impl_::StandardScoreController, typename ParamContainer, typename OutputBlob, typename Result, typename DBAdapter>
	void cmdProcessRem(ParamContainer const &p, DBAdapter &db, Result &result, OutputBlob &, size_t scoreSize){
		// REM key subkey0 subkey1 ...

		if (p.size() < 3)
			return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_3);

		const auto &keyN = p[1];

		if (keyN.empty())
			return result.set_error(ResultErrorMessages::EMPTY_KEY);

		auto const varg = 2;

		for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
			if (auto const &keySub = *itk; !isKeyValid(keyN, keySub, scoreSize))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
		}

		for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
			auto const &keySub = *itk;

			rem<SC>(db, keyN, keySub);
		}

		return result.set_1();
	}

} // net::worker::shared::zset

#endif

