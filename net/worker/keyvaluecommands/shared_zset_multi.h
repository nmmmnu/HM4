#ifndef SHARED_ZSET_MULTI_H_
#define SHARED_ZSET_MULTI_H_

#include "mystring.h"
#include "stringtokenizer.h"

namespace net::worker::shared::zsetmulti{
	template<size_t N>
	bool valid(std::array<std::string_view, N> const &indexes){
		for(auto const &x : indexes)
			if (x.empty())
				return false;

		return true;
	}

	template<size_t N>
	auto makeACopy(std::array<std::string_view, N> const &src){
		struct Storage{
			std::array<std::string_view,   N> copy;
			std::array<hm4::PairBufferKey, N> buffer;
		};

		Storage x;

		for(size_t i = 0; i < N; ++i)
			x.copy[i] = concatenateBuffer(x.buffer[i], src[i]);

		return x;
	}

	constexpr size_t prefixSize(std::string_view keyN, std::string_view txt){
		// keyN~_~A~, 3 * ~ + keyN + _
		return 3 * 1 + keyN.size() + txt.size();
	}

	struct Permutation1{
		constexpr static size_t N = 1;

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, size_t more = 0){
			// keyN~_~A~keySub, 3 * ~ + _
			return hm4::Pair::isCompositeKeyValid(3 * 1 + 1 + more, keyN, keySub);
		}

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, size_t more = 0){
			// keyN~_~A~keySub, 3 * ~ + _
			return hm4::Pair::isCompositeKeyValid(3 * 1 + 1 + more, keyN, keySub, indexes[0]);
		}

		static auto encodeIndex(std::string_view /* separator */, std::array<std::string_view, N> const &indexes, hm4::PairBufferKey bufferKey){
			// no need to copy, but lets do it anyway, because the caller expects it.
			return concatenateBuffer(bufferKey,
						indexes[0]
			);
		}

		static auto decodeIndex(std::string_view separator, std::string_view s){
			StringTokenizer const tok{ s, separator[0] };
			auto _ = getForwardTokenizer(tok);

			return std::array<std::string_view, N>{ _() };
		}

		static std::string_view makeKey(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view key,
					std::string_view txt,
					std::string_view a = "", std::string_view b = ""){

			uint64_t const x =
				(a.empty() ? 0x00 : 0xA0) |
				(b.empty() ? 0x00 : 0x0B) |
				0;

			switch(x){
			default:
			case 0x00:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator
				);

			case 0xA0:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator
				);

			case 0xAB:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b
				);
			}
		}

		static std::string_view makeKeyNoSeparator(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view key,
					std::string_view txt,
					std::string_view a = "", std::string_view b = ""){

			uint64_t const x =
				(a.empty() ? 0x00 : 0xA0) |
				(b.empty() ? 0x00 : 0x0B) |
				0;

			switch(x){
			default:
			case 0x00:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator
				);

			case 0xA0:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a
				);

			case 0xAB:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b
				);
			}
		}

		static std::string_view makeKeyCtrl(hm4::PairBufferKey &bufferKey, std::string_view separator,
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
					std::array<std::string_view, N> const &indexes){

			return concatenateBuffer(bufferKey,
					keyN		,	separator	,
					"A"		,	separator	,
					indexes[0]	,	separator	,
					keySub
			);
		}

		template<typename F>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, F f){
			auto const S = keySub;
			auto const A = indexes[0];

			auto ff = [&](std::string_view txt, std::string_view a, std::string_view b){
				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b);

				f(key);
			};

			ff("A", A, S);
		}
	};



	struct Permutation2{
		constexpr static size_t N = 2;
		constexpr static inline std::string_view MAIN_INDEX = "AB";

		constexpr static bool valid(std::string_view keyN, std::string_view keySub){
			// keyN~AB~A~B~keySub, 4 * ~ + AB
			return hm4::Pair::isCompositeKeyValid(4 * 1 + 2, keyN, keySub);
		}

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes){
			// keyN~AB~A~B~keySub, 4 * ~ + AB
			return hm4::Pair::isCompositeKeyValid(4 * 1 + 2, keyN, keySub, indexes[0], indexes[1]);
		}

		static auto encodeIndex(std::string_view separator, std::array<std::string_view, N> const &indexes, hm4::PairBufferKey bufferKey){
			// no need to copy, but lets do it anyway, because the caller expects it.
			return concatenateBuffer(bufferKey,
						indexes[0],	separator	,
						indexes[1]
			);
		}

		static auto decodeIndex(std::string_view separator, std::string_view s){
			StringTokenizer const tok{ s, separator[0] };
			auto _ = getForwardTokenizer(tok);

			return std::array<std::string_view, N>{ _(), _() };
		}

		static std::string_view makeKey(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view key,
					std::string_view txt,
					std::string_view a = "", std::string_view b = "", std::string_view c = ""){

			uint64_t const x =
				(a.empty() ? 0x000 : 0xA00) |
				(b.empty() ? 0x000 : 0x0B0) |
				(c.empty() ? 0x000 : 0x00C) |
				0;

			switch(x){
			default:
			case 0x000:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator
				);

			case 0xA00:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator
				);

			case 0xAB0:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator
				);

			case 0xABC:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c
				);
			}
		}

		template<typename F>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, F f){
			auto const S = keySub;
			auto const A = indexes[0];
			auto const B = indexes[1];

			auto ff = [&](std::string_view txt, std::string_view a, std::string_view b, std::string_view c){
				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b, c);

				f(key);
			};

			ff("",   S, A, B);
			ff("AB", A, B, S);
			ff("BA", B, A, S);
		}
	};



	struct Permutation3{
		constexpr static size_t N = 3;
		constexpr static inline std::string_view MAIN_INDEX = "ABC";

		constexpr static bool valid(std::string_view keyN, std::string_view keySub){
			// keyN~ABC~A~B~C~keySub, 5 * ~ + ABC
			return hm4::Pair::isCompositeKeyValid(5 * 1 + 3, keyN, keySub);
		}

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes){
			// keyN~ABC~A~B~C~keySub, 5 * ~ + ABC
			return hm4::Pair::isCompositeKeyValid(5 * 1 + 3, keyN, keySub, indexes[0], indexes[1], indexes[2]);
		}

		static auto encodeIndex(std::string_view separator, std::array<std::string_view, N> const &indexes, hm4::PairBufferKey bufferKey){
			// no need to copy, but lets do it anyway, because the caller expects it.
			return concatenateBuffer(bufferKey,
						indexes[0],	separator	,
						indexes[1],	separator	,
						indexes[2]
			);
		}

		static auto decodeIndex(std::string_view separator, std::string_view s){
			StringTokenizer const tok{ s, separator[0] };
			auto _ = getForwardTokenizer(tok);

			return std::array<std::string_view, N>{ _(), _(), _() };
		}

		static std::string_view makeKey(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view key,
					std::string_view txt,
					std::string_view a = "", std::string_view b = "", std::string_view c = "", std::string_view d = ""){

			uint64_t const x =
				(a.empty() ? 0x0000 : 0xA000) |
				(b.empty() ? 0x0000 : 0x0B00) |
				(c.empty() ? 0x0000 : 0x00C0) |
				(d.empty() ? 0x0000 : 0x000D) |
				0;

			switch(x){
			default:
			case 0x0000:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator
				);

			case 0xA000:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator
				);

			case 0xAB00:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator
				);

			case 0xABC0:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator
				);

			case 0xABCD:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator	,
						d
				);
			}
		}

		template<typename F>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, F f){
			auto const S = keySub;
			auto const A = indexes[0];
			auto const B = indexes[1];
			auto const C = indexes[2];

			auto ff = [&](std::string_view txt, std::string_view a, std::string_view b, std::string_view c, std::string_view d){
				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b, c, d);

				f(key);
			};

			ff("ABC", A, B, C, S);
			ff("ACB", A, C, B, S);
			ff("BAC", B, A, C, S);
			ff("BCA", B, C, A, S);
			ff("CAB", C, A, B, S);
			ff("CBA", C, B, A, S);

		}
	};



	template<typename Permutation, typename DBAdapter>
	void add(DBAdapter &db,
			std::string_view keyN, std::string_view keySub, std::array<std::string_view, Permutation::N> const &indexes, std::string_view value){

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = Permutation::makeKey(bufferKeyCtrl, DBAdapter::SEPARATOR, keyN, "", keySub);

		logger<Logger::DEBUG>() << "ZSetMulti::ADD: ctrl key" << keyCtrl;

		{
			// Update control key and delete hash key if any

			if (auto const pair = hm4::getPairPtr(*db, keyCtrl); pair){
				// Case 1: ctrl key is set

				auto const indexesOld = Permutation::decodeIndex(DBAdapter::SEPARATOR, pair->getVal());

				if (!valid(indexesOld)){
					// Case 1.0: invalid ctrl key, probable attack.

					logger<Logger::DEBUG>() << "ZSetMulti::ADD: INVALID ctrl key" << keyCtrl;

					// HINT
					const auto *hint = pair;
					hm4::insertHintF<hm4::PairFactory::Tombstone>(*db, hint, keyCtrl);
				}else if (indexesOld == indexes){
					// Case 1.1: ctrl key is same, no need to be updated.

					logger<Logger::DEBUG>() << "ZSetMulti::ADD: SKIP SET ctrl key" << keyCtrl;
				}else{
					// Case 1.2: old ctrl key has to be updated

					auto const save = makeACopy(indexesOld);

					auto deleteOldKeys = [&](std::string_view key){
						logger<Logger::DEBUG>() << "ZSetMulti::ADD: DEL old index key" << key;

						erase(*db, key);
					};

					Permutation::template for_each(DBAdapter::SEPARATOR, keyN, keySub, save.copy, deleteOldKeys);
				}
			}else{
				// Case 2: no ctrl key

				logger<Logger::DEBUG>() << "ZSetMulti::ADD: no ctrl key" << keyCtrl;
			}
		}

		auto insertNewKeys = [&](std::string_view key){
			logger<Logger::DEBUG>() << "ZSetMulti::ADD: SET index key" << key;

			insert(*db, key, value);
		};

		Permutation::for_each(DBAdapter::SEPARATOR, keyN, keySub, indexes, insertNewKeys);

		logger<Logger::DEBUG>() << "ZSetMulti::ADD: SET ctrl key" << keyCtrl;

		hm4::PairBufferKey bufferVal;
		auto const encodedValue =  Permutation::encodeIndex(DBAdapter::SEPARATOR, indexes, bufferVal);

		insert(*db, keyCtrl, encodedValue);

	}



	template<typename Permutation, typename DBAdapter>
	void rem(DBAdapter &db,
			std::string_view keyN, std::string_view keySub){

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = Permutation::makeKey(bufferKeyCtrl, DBAdapter::SEPARATOR, keyN, "", keySub);

		logger<Logger::DEBUG>() << "ZSetMulti::REM: ctrl key" << keyCtrl;

		if (auto const pair = hm4::getPairPtr(*db, keyCtrl); pair){
			// Case 1: ctrl key is set

			auto const indexesOld = Permutation::decodeIndex(DBAdapter::SEPARATOR, pair->getVal());

			if (!valid(indexesOld)){
				// Case 1.0: invalid ctrl key, probable attack.

				logger<Logger::DEBUG>() << "ZSetMulti::REM: INVALID ctrl key" << keyCtrl;

				// HINT
				const auto *hint = pair;
				hm4::insertHintF<hm4::PairFactory::Tombstone>(*db, hint, keyCtrl);
			}else{
				// Case 1.1: old ctrl key has to be removed

				hm4::PairBufferKey bufferKeySave[2];

				// make a copy, because values may be invalidated.
				using ArrayN = std::array<std::string_view, Permutation::N>;

				ArrayN indexesOldSave;

				for(size_t i = 0; i < Permutation::N; ++i)
					indexesOldSave[i] = concatenateBuffer(bufferKeySave[i], indexesOld[i]);

				auto deleteOldKeys = [&](std::string_view key){
					logger<Logger::DEBUG>() << "ZSetMulti::REM: DEL old index key" << key;

					erase(*db, key);
				};

				Permutation::for_each(DBAdapter::SEPARATOR, keyN, keySub, indexesOldSave, deleteOldKeys);

				logger<Logger::DEBUG>() << "ZSetMulti::REM: DEL ctrl key" << keyCtrl;

				erase(*db, keyCtrl);
			}
		}else{
			// Case 2: no ctrl key

			logger<Logger::DEBUG>() << "ZSetMulti::REM: no ctrl key" << keyCtrl;
		}
	}



	template<typename Permutation, typename DBAdapter>
	std::string_view get(DBAdapter &db,
			std::string_view keyN, std::string_view keySub){

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = Permutation::makeKey(bufferKeyCtrl, DBAdapter::SEPARATOR, keyN, "", keySub);

		logger<Logger::DEBUG>() << "ZSetMulti::GET: ctrl key" << keyCtrl;

		if (auto const encodedValue = hm4::getPairVal(*db, keyCtrl); !encodedValue.empty()){
			// Case 1: ctrl key is set

			auto const indexes = Permutation::decodeIndex(DBAdapter::SEPARATOR, encodedValue);

			if (Permutation::valid(keyN, keySub, indexes)){
				hm4::PairBufferKey bufferKeyData;
				auto const keyData = Permutation::makeKeyData(bufferKeyData, DBAdapter::SEPARATOR, keyN, keySub, indexes);

				logger<Logger::DEBUG>() << "ZSetMulti::GET: data key" << keyData;

				return hm4::getPairVal(*db, keyData);
			}
		}

		return "";
	}



	template<typename Permutation, typename DBAdapter>
	std::array<std::string_view, Permutation::N> getIndexes(DBAdapter &db,
			std::string_view keyN, std::string_view keySub){

		hm4::PairBufferKey bufferKeyPrefix;
		auto const keyPrefix = Permutation::makeKey(bufferKeyPrefix, DBAdapter::SEPARATOR, keyN, "", keySub);

		logger<Logger::DEBUG>() << "ZSetMulti::GET_INDEX: prefix key" << keyPrefix;

		if (const auto *pair = hm4::getPairPtr/*ByPrefix*/(*db, keyPrefix); pair){
			// Case 1: ctrl key is set

			auto const keyCtrl = pair->getKey();

			auto const ixKey = after_prefix(keyPrefix, keyCtrl);
			return Permutation::decodeIndex(DBAdapter::SEPARATOR, ixKey);
		}

		return {};
	}



	template<typename Permutation, typename DBAdapter>
	bool exists(DBAdapter &db,
			std::string_view keyN, std::string_view keySub){

		hm4::PairBufferKey bufferKeyPrefix;
		auto const keyPrefix = Permutation::makeKey(bufferKeyPrefix, DBAdapter::SEPARATOR, keyN, "", keySub);

		logger<Logger::DEBUG>() << "ZSetMulti::EXISTS: prefix key" << keyPrefix;

		return hm4::getPairOK/*ByPrefix*/(*db, keyPrefix);
	}



	template<typename Permutation, typename ParamContainer, typename OutputBlob, typename Result, typename DBAdapter>
	void cmdProcessExists(ParamContainer const &p, DBAdapter &db, Result &result, OutputBlob &){
		// EXISTS key subkey0

		if (p.size() != 3)
			return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

		auto const &keyN   = p[1];
		auto const &keySub = p[2];

		if (keyN.empty() || keySub.empty())
			return result.set_error(ResultErrorMessages::EMPTY_KEY);

		if (!Permutation::valid(keyN, keySub))
			return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

		return result.set(
			exists<Permutation>(db, keyN, keySub)
		);
	}

	template<typename Permutation, typename ParamContainer, typename OutputBlob, typename Result, typename DBAdapter>
	void cmdProcessRem(ParamContainer const &p, DBAdapter &db, Result &result, OutputBlob &){
		// REM key subkey0 subkey1 ...

		if (p.size() < 3)
			return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_3);

		const auto &keyN = p[1];

		if (keyN.empty())
			return result.set_error(ResultErrorMessages::EMPTY_KEY);

		auto const varg = 2;

		for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
			if (auto const &keySub = *itk; !Permutation::valid(keyN, keySub))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
		}

		for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
			auto const &keySub = *itk;

			rem<Permutation>(db, keyN, keySub);
		}

		return result.set_1();
	}

} // net::worker::shared::zset

#endif

