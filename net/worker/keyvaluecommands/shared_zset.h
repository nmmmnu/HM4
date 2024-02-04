#ifndef SHARED_ZSET_H_
#define SHARED_ZSET_H_

#include "mystring.h"

namespace net::worker::shared::zset{

	constexpr bool isKeyValid(std::string_view keyN, std::string_view keySub, size_t score_size){
		return hm4::Pair::isKeyValid(
			keyN.size()		+
			1			+
			keySub.size()		+
			1			+
			score_size
		);
	}

	constexpr bool isKeyValid(std::string_view keyN, std::string_view keySub, std::string_view score){
		return isKeyValid(keyN, keySub, score.size());
	}



	namespace impl_{
		template<typename DBAdapter>
		std::string_view makeKey(DBAdapter &,
				std::string_view keyN, std::string_view keySub, std::string_view score,
				hm4::PairBufferKey &bufferKey){

			return concatenateBuffer(bufferKey,
						keyN			,
						DBAdapter::SEPARATOR	,
						score			,
						DBAdapter::SEPARATOR	,
						keySub
			);
		}

		template<typename DBAdapter>
		std::string_view makeKey(DBAdapter &,
				std::string_view keyN, std::string_view keySub,
				hm4::PairBufferKey &bufferKey){

			return concatenateBuffer(bufferKey,
						keyN			,
						DBAdapter::SEPARATOR	,
						DBAdapter::SEPARATOR	,
						keySub
			);
		}



		struct StandardScoreController{
			constexpr static bool canExtractValue = false;

			constexpr static std::string_view decode(std::string_view score){
				return score;
			}

			constexpr static std::string_view encode(std::string_view score, std::string_view){
				return score;
			}
		};



		template<typename SC = StandardScoreController, typename DBAdapter>
		std::string_view getControlKeyNoCheck(DBAdapter &db, std::string_view keyCtrl){
			return SC::decode( hm4::getPairVal(*db, keyCtrl) );
		}
	} // namespace impl_



	template<typename SC = impl_::StandardScoreController, typename DBAdapter>
	void add(DBAdapter &db,
			std::string_view keyN, std::string_view keySub, std::string_view score, std::string_view value){

		using namespace impl_;

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKey(db, keyN, keySub, bufferKeyCtrl);

		logger<Logger::DEBUG>() << "ZSet GET ctrl key" << keyCtrl;

		if (auto const scoreOld = getControlKeyNoCheck<SC>(db, keyCtrl); !scoreOld.empty()){
			// Case 1: ctrl key is set

			if (!isKeyValid(keyN, keySub, scoreOld)){
				// Case 1.0: invalid ctrl key, probable attack.
				logger<Logger::DEBUG>() << "ZSet INVALID SET ctrl key" << keyCtrl;

				insert(*db, keyCtrl, SC::encode(score, value));
			}else if (scoreOld == score){
				// Case 1.1: ctrl key is same, no need to be updated.

				logger<Logger::DEBUG>() << "ZSet SKIP SET ctrl key";
			}else{
				// Case 1.2: old ctrl key has to be updated

				hm4::PairBufferKey bufferKeyHashOld;
				auto const keyHashOld = makeKey(db, keyN, keySub, scoreOld, bufferKeyHashOld);

				logger<Logger::DEBUG>() << "ZSet DEL old hash key" << keyHashOld;

				erase(*db, keyHashOld);

				logger<Logger::DEBUG>() << "ZSet SET ctrl key" << keyCtrl;

				insert(*db, keyCtrl, SC::encode(score, value));
			}
		}else{
			// Case 2: no ctrl key

			logger<Logger::DEBUG>() << "ZSet SET ctrl key" << keyCtrl;

			insert(*db, keyCtrl, SC::encode(score, value));
		}

		hm4::PairBufferKey bufferKeyHash;
		auto const keyHash = makeKey(db, keyN, keySub, score, bufferKeyHash);

		logger<Logger::DEBUG>() << "ZSet SET hash key" << keyHash;

		insert(*db, keyHash, value);
	}

	template<typename SC = impl_::StandardScoreController, typename DBAdapter>
	void rem(DBAdapter &db,
			std::string_view keyN, std::string_view keySub){

		using namespace impl_;

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKey(db, keyN, keySub, bufferKeyCtrl);

		logger<Logger::DEBUG>() << "ZSet GET ctrl key" << keyCtrl;

		if (auto const score = getControlKeyNoCheck<SC>(db, keyCtrl); !score.empty()){
			// Case 1: ctrl key is set

			if (!isKeyValid(keyN, keySub, score)){
				// Case 1.0: invalid ctrl key, probable attack.
				logger<Logger::DEBUG>() << "ZSet INVALID ctrl key" << keyCtrl;

				erase(*db, keyCtrl);
			}else{
				// Case 1.1: ctrl key has to be removed

				hm4::PairBufferKey bufferKeyHash;
				auto const keyHash = makeKey(db, keyN, keySub, score, bufferKeyHash);

				logger<Logger::DEBUG>() << "ZSet DEL ctrl key" << keyCtrl;

				erase(*db, keyCtrl);

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
		auto const keyCtrl = makeKey(db, keyN, keySub, bufferKeyCtrl);

		logger<Logger::DEBUG>() << "ZSet GET ctrl key" << keyCtrl;

		if (auto const score = getControlKeyNoCheck<SC>(db, keyCtrl); !score.empty()){
			// Case 1: ctrl key is set

			if (!isKeyValid(keyN, keySub, score)){
				// Case 1.0: invalid ctrl key, probable attack.
				logger<Logger::DEBUG>() << "ZSet INVALID ctrl key" << keyCtrl;

				return "";
			}

			// Case 1.1: valid ctrl key.

			if constexpr(SC::canExtractValue){
				logger<Logger::DEBUG>() << "ZSet SKIP hash key by smart controller";

				return SC::getValue(score);
			}else{
				hm4::PairBufferKey bufferKeyHash;
				auto const keyHash = makeKey(db, keyN, keySub, score, bufferKeyHash);

				logger<Logger::DEBUG>() << "ZSet GET hash key" << keyHash;

				return hm4::getPairVal(*db, keyHash);
			}
		}else{
			// Case 2: no ctrl key

			logger<Logger::DEBUG>() << "ZSet no ctrl key";

			return "";
		}
	}

	template<typename SC = impl_::StandardScoreController, typename DBAdapter>
	bool exists(DBAdapter &db,
			std::string_view keyN, std::string_view keySub){

		using namespace impl_;

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKey(db, keyN, keySub, bufferKeyCtrl);

		logger<Logger::DEBUG>() << "ZSet GET ctrl key" << keyCtrl;

		if (auto const score = getControlKeyNoCheck<SC>(db, keyCtrl); !score.empty()){
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

	template<typename SC = impl_::StandardScoreController, typename DBAdapter>
	std::string_view score(DBAdapter &db,
			std::string_view keyN, std::string_view keySub){

		using namespace impl_;

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKey(db, keyN, keySub, bufferKeyCtrl);

		logger<Logger::DEBUG>() << "ZScore GET ctrl key" << keyCtrl;

		if (auto const score = getControlKeyNoCheck<SC>(db, keyCtrl); !score.empty()){
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



	template<typename SC = impl_::StandardScoreController, typename ParamContainer, typename OutputBlob, typename Result, typename DBAdapter>
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

