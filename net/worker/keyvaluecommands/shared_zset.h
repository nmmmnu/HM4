#ifndef SHARED_ZSET_H_
#define SHARED_ZSET_H_

//#include "hexconvert.h"
//#include "stringtokenizer.h"

namespace net::worker::shared::zset{

	constexpr bool isZKeyValid(std::string_view keyN, std::string_view subKey, size_t score_size){
		return hm4::Pair::isKeyValid(
			keyN.size()		+
			1			+
			subKey.size()		+
			1			+
			score_size
		);
	}

	constexpr bool isZKeyValid(std::string_view keyN, std::string_view subKey, std::string_view score){
		return isZKeyValid(keyN, subKey, score.size());
	}



	template<typename DBAdapter, typename BufferKey>
	void zAdd(DBAdapter &db,
			std::string_view keyN, std::string_view subKey, std::string_view score, std::string_view value,
			BufferKey &buffer_key_ctrl, BufferKey &buffer_key_hash){

		auto const keyCtrl = concatenateBuffer(buffer_key_ctrl,
					keyN			,
					DBAdapter::SEPARATOR	,
					DBAdapter::SEPARATOR	,
					subKey
		);

		auto const keyHash = concatenateBuffer(buffer_key_hash,
					keyN			,
					DBAdapter::SEPARATOR	,
					score			,
					DBAdapter::SEPARATOR	,
					subKey
		);

		logger<Logger::DEBUG>() << "ZSet GET ctrl key" << keyCtrl;

		if (auto const keyHashOld = hm4::getPairVal(*db, keyCtrl); !keyHashOld.empty()){
			// Case 1: ctrl key is set

			if (keyHashOld != keyHash){
				// Case 1.1: ctrl key has to be updated

				logger<Logger::DEBUG>() << "ZSet DEL old hash key" << keyHashOld;

				erase(*db, keyHashOld);

				// keyHashOld may be dangled now,
				// but we do not need it anymore

				logger<Logger::DEBUG>() << "ZSet SET ctrl key" << keyCtrl;

				insert(*db, keyCtrl, keyHash);
			}else{
				// Case 1.2: ctrl key is same, no need to be updated.

				logger<Logger::DEBUG>() << "ZSet SKIP SET ctrl key";
			}
		}else{
			// Case 2: no ctrl key

			logger<Logger::DEBUG>() << "ZSet SET ctrl key" << keyCtrl;

			insert(*db, keyCtrl, keyHash);
		}

		logger<Logger::DEBUG>() << "ZSet SET hash key" << keyHash;

		insert(*db, keyHash, value);
	}

	template<typename DBAdapter, typename BufferKey>
	void zRem(DBAdapter &db,
			std::string_view keyN, std::string_view subKey,
			BufferKey &buffer_key_ctrl){

		auto const keyCtrl = concatenateBuffer(buffer_key_ctrl,
					keyN			,
					DBAdapter::SEPARATOR	,
					DBAdapter::SEPARATOR	,
					subKey
		);

		logger<Logger::DEBUG>() << "ZSet GET ctrl key" << keyCtrl;

		if (auto const keyHash = hm4::getPairVal(*db, keyCtrl); !keyHash.empty()){
			// Case 1: ctrl key is set

			logger<Logger::DEBUG>() << "ZSet DEL hash key" << keyHash;

			erase(*db, keyHash);

			// keyHash may be dangled now,
			// but we do not need it anymore

			logger<Logger::DEBUG>() << "ZSet DEL ctrl key" << keyCtrl;

			erase(*db, keyCtrl);
		}else{
			// Case 2: no ctrl key

			logger<Logger::DEBUG>() << "ZSet no ctrl key";
		}
	}

	template<typename DBAdapter, typename BufferKey>
	std::string_view zGet(DBAdapter &db,
			std::string_view keyN, std::string_view subKey,
			BufferKey &buffer_key_ctrl){

		auto const keyCtrl = concatenateBuffer(buffer_key_ctrl,
					keyN			,
					DBAdapter::SEPARATOR	,
					DBAdapter::SEPARATOR	,
					subKey
		);

		logger<Logger::DEBUG>() << "ZSet ctrl key" << keyCtrl;

		if (auto const keyHash = hm4::getPairVal(*db, keyCtrl); !keyHash.empty()){
			// Case 1: ctrl key is set

			logger<Logger::DEBUG>() << "ZSet GET hash key" << keyHash;

			return hm4::getPairVal(*db, keyHash);
		}else{
			// Case 2: no ctrl key

			logger<Logger::DEBUG>() << "ZSet no ctrl key";

			return "";
		}
	}

} // net::worker::shared::zset

#endif

