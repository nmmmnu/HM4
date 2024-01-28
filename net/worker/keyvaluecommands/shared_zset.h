#ifndef SHARED_ZSET_H_
#define SHARED_ZSET_H_

#include "mystring.h"

namespace net::worker::shared::zset{

	constexpr bool isKeyValid(std::string_view keyN, std::string_view subKey, size_t score_size){
		return hm4::Pair::isKeyValid(
			keyN.size()		+
			1			+
			subKey.size()		+
			1			+
			score_size
		);
	}

	constexpr bool isKeyValid(std::string_view keyN, std::string_view subKey, std::string_view score){
		return isKeyValid(keyN, subKey, score.size());
	}



	namespace impl_{
		template<typename DBAdapter, typename BufferKey>
		std::string_view makeKey(DBAdapter &,
				std::string_view keyN, std::string_view subKey, std::string_view score,
				BufferKey &buffer_key){

			return concatenateBuffer(buffer_key,
						keyN			,
						DBAdapter::SEPARATOR	,
						score			,
						DBAdapter::SEPARATOR	,
						subKey
			);
		}

		template<typename DBAdapter, typename BufferKey>
		std::string_view makeKey(DBAdapter &,
				std::string_view keyN, std::string_view subKey,
				BufferKey &buffer_key){

			return concatenateBuffer(buffer_key,
						keyN			,
						DBAdapter::SEPARATOR	,
						DBAdapter::SEPARATOR	,
						subKey
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
		std::string_view getControlKey(DBAdapter &db, std::string_view keyCtrl){
			return SC::decode(
					hm4::getPairVal(*db, keyCtrl)
			);
		}
	} // namespace impl_



	template<typename SC = impl_::StandardScoreController, typename DBAdapter, typename BufferKey>
	void add(DBAdapter &db,
			std::string_view keyN, std::string_view subKey, std::string_view score, std::string_view value,
			BufferKey &buffer_key_ctrl, BufferKey &buffer_key_hash){

		using namespace impl_;

		auto const keyCtrl = makeKey(db, keyN, subKey, buffer_key_ctrl);

		logger<Logger::DEBUG>() << "ZSet GET ctrl key" << keyCtrl;

		if (auto const scoreOld = getControlKey<SC>(db, keyCtrl); !scoreOld.empty()){
			// Case 1: ctrl key is set

			if (scoreOld != score){
				// Case 1.1: old ctrl key has to be updated

				auto const keyHashOld = makeKey(db, keyN, subKey, scoreOld, buffer_key_hash);

				logger<Logger::DEBUG>() << "ZSet DEL old hash key" << keyHashOld;

				erase(*db, keyHashOld);

				logger<Logger::DEBUG>() << "ZSet SET ctrl key" << keyCtrl;

				insert(*db, keyCtrl, SC::encode(score, value));
			}else{
				// Case 1.2: ctrl key is same, no need to be updated.

				logger<Logger::DEBUG>() << "ZSet SKIP SET ctrl key";
			}
		}else{
			// Case 2: no ctrl key

			logger<Logger::DEBUG>() << "ZSet SET ctrl key" << keyCtrl;

			insert(*db, keyCtrl, SC::encode(score, value));
		}

		auto const keyHash = makeKey(db, keyN, subKey, score, buffer_key_hash);

		logger<Logger::DEBUG>() << "ZSet SET hash key" << keyHash;

		insert(*db, keyHash, value);
	}

	template<typename SC = impl_::StandardScoreController, typename DBAdapter, typename BufferKey>
	void rem(DBAdapter &db,
			std::string_view keyN, std::string_view subKey,
			BufferKey &buffer_key_ctrl, BufferKey &buffer_key_hash){

		using namespace impl_;

		auto const keyCtrl = makeKey(db, keyN, subKey, buffer_key_ctrl);

		logger<Logger::DEBUG>() << "ZSet GET ctrl key" << keyCtrl;

		if (auto const score = getControlKey<SC>(db, keyCtrl); !score.empty()){
			// Case 1: ctrl key is set

			auto const keyHash = makeKey(db, keyN, subKey, score, buffer_key_hash);

			logger<Logger::DEBUG>() << "ZSet DEL ctrl key" << keyCtrl;

			erase(*db, keyCtrl);

			logger<Logger::DEBUG>() << "ZSet DEL hash key" << keyHash;

			erase(*db, keyHash);
		}else{
			// Case 2: no ctrl key

			logger<Logger::DEBUG>() << "ZSet no ctrl key";
		}
	}

	template<typename SC = impl_::StandardScoreController, typename DBAdapter, typename BufferKey>
	std::string_view get(DBAdapter &db,
			std::string_view keyN, std::string_view subKey,
			BufferKey &buffer_key){

		using namespace impl_;

		auto const keyCtrl = makeKey(db, keyN, subKey, buffer_key);

		logger<Logger::DEBUG>() << "ZSet GET ctrl key" << keyCtrl;

		if (auto const score = getControlKey<SC>(db, keyCtrl); !score.empty()){
			// Case 1: ctrl key is set

			if constexpr(SC::canExtractValue){
				logger<Logger::DEBUG>() << "ZSet SKIP hash key by smart controller";

				return SC::getValue(score);
			}else{
				auto const keyHash = makeKey(db, keyN, subKey, score, buffer_key);

				logger<Logger::DEBUG>() << "ZSet GET hash key" << keyHash;

				return hm4::getPairVal(*db, keyHash);
			}
		}else{
			// Case 2: no ctrl key

			logger<Logger::DEBUG>() << "ZSet no ctrl key";

			return "";
		}
	}

	template<typename SC = impl_::StandardScoreController, typename DBAdapter, typename BufferKey>
	bool exists(DBAdapter &db,
			std::string_view keyN, std::string_view subKey,
			BufferKey &buffer_key){

		using namespace impl_;

		auto const keyCtrl = makeKey(db, keyN, subKey, buffer_key);

		logger<Logger::DEBUG>() << "ZSet GET ctrl key" << keyCtrl;

		auto const score = getControlKey<SC>(db, keyCtrl);

		return !score.empty();
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

	template<typename SC = impl_::StandardScoreController, typename DBAdapter, typename BufferKey>
	std::string_view score(DBAdapter &db,
			std::string_view keyN, std::string_view subKey,
			BufferKey &buffer_key_ctrl){

		using namespace impl_;

		auto const keyCtrl = makeKey(db, keyN, subKey, buffer_key_ctrl);

		logger<Logger::DEBUG>() << "ZScore GET ctrl key" << keyCtrl;

		if (auto const score = getControlKey<SC>(db, keyCtrl); !score.empty())
			return score;
		else
			return "";
	}

} // net::worker::shared::zset

#endif

