#include "base.h"
#include "pair_vfactory.h"
#include "mytime.h"
#include "ilist/txguard.h"

namespace net::worker::commands::Mutable{



	template<class Protocol, class DBAdapter>
	struct SET : BaseCommandRW<Protocol,DBAdapter>{

		SET() : BaseCommandRW<Protocol,DBAdapter>("SET", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 3 && p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_23);

			auto const &key = p[1];
			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const &val = p[2];
			if (!hm4::Pair::isValValid(val))
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			auto const exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

			hm4::insert(*db, key, val, exp);

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"set",		"SET"
		};

	};



	template<class Protocol, class DBAdapter>
	struct MSET : BaseCommandRW<Protocol,DBAdapter>{

		MSET() : BaseCommandRW<Protocol,DBAdapter>("MSET", std::begin(cmd__), std::end(cmd__)){}

		// MSET a 5 b 6
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			auto const varg = 1;

			if (p.size() < 3 || (p.size() - varg) % 2 != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_2);


			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				if (const auto &key = *itk;            !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				if (const auto &val = *std::next(itk); !hm4::Pair::isValValid(val))
					return result.set_error(ResultErrorMessages::EMPTY_VAL);
			}

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &key = *itk;
				auto const &val = *std::next(itk);

				hm4::insert(*db, key, val);
			}

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"mset",		"MSET"
		};

	};



	template<class Protocol, class DBAdapter>
	struct MSETNX : BaseCommandRW<Protocol,DBAdapter>{

		MSETNX() : BaseCommandRW<Protocol,DBAdapter>("MSETNX", std::begin(cmd__), std::end(cmd__)){}

		// MSET a 5 b 6
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			auto const varg = 1;

			if (p.size() < 3 || (p.size() - varg) % 2 != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_2);


			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				if (const auto &key = *itk;            !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				if (const auto &val = *std::next(itk); !hm4::Pair::isValValid(val))
					return result.set_error(ResultErrorMessages::EMPTY_VAL);
			}

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			// check if any key exists
			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &key = *itk;

				if ( hm4::getPairOK(*db, key) )
					return result.set_0();
			}

			// set keys
			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &key = *itk;
				auto const &val = *std::next(itk);

				hm4::insert(*db, key, val);
			}

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"msetnx",		"MSETNX"
		};

	};



	template<class Protocol, class DBAdapter>
	struct MSETXX : BaseCommandRW<Protocol,DBAdapter>{

		MSETXX() : BaseCommandRW<Protocol,DBAdapter>("MSETXX", std::begin(cmd__), std::end(cmd__)){}

		// MSETXX a 5 b 6
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			using hm4::Pair;

			auto const varg = 1;

			if (p.size() < 3 || (p.size() - varg) % 2 != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_2);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				if (const auto &key = *itk;            !Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				if (const auto &val = *std::next(itk); !Pair::isValValid(val))
					return result.set_error(ResultErrorMessages::EMPTY_VAL);
			}

			PairVector pcontainer;

			// theoretically can happen
			if (p.size() / 2 > pcontainer.capacity())
				return result.set_error(ResultErrorMessages::CONTAINER_CAPACITY);

			// check if any key NOT exists
			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &key = *itk;
				auto const &val = *std::next(itk);

				if (auto *it = hm4::getPairPtr(*db, key); it){
					if (const auto *hint = & *it; hm4::canInsertHintValSize(*db, hint, val.size())){
						// HINT
						pcontainer.push_back(hint);
					}else
						pcontainer.push_back(nullptr);
				}else
					return result.set_0();
			}

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			// HINT
			for(size_t i = 0; i < pcontainer.size(); ++i){
				if (const auto *hint = pcontainer[i]; hint){
					auto const &key = p[varg + i * 2 + 0];
					auto const &val = p[varg + i * 2 + 1];

					assert(key == hint->getKey());

					hm4::proceedInsertHintF<hm4::PairFactory::Normal>(*db, const_cast<Pair *>(hint), key, val);
				}
			}

			// NORMAL
			for(size_t i = 0; i < pcontainer.size(); ++i){
				if (const auto *hint = pcontainer[i]; !hint){
					auto const &key = p[varg + i * 2 + 0];
					auto const &val = p[varg + i * 2 + 1];

					hm4::insert(*db, key, val);
				}
			}

			return result.set_1();
		}

	private:
		using PairVector = StaticVector<const hm4::Pair *, OutputBlob::ParamContainerSize>;

	private:
		constexpr inline static std::string_view cmd__[] = {
			"msetxx",		"MSETXX"
		};

	};



	template<class Protocol, class DBAdapter>
	struct SETEX : BaseCommandRW<Protocol,DBAdapter>{

		SETEX() : BaseCommandRW<Protocol,DBAdapter>("SETEX", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &key = p[1];
			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const &val = p[3];
			if (!hm4::Pair::isValValid(val))
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			auto const exp  = from_string<uint32_t>(p[2]);

			hm4::insert(*db, key, val, exp);

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"setex",	"SETEX"
		};

	};



	template<class Protocol, class DBAdapter>
	struct HSET : BaseCommandRW<Protocol,DBAdapter>{

		HSET() : BaseCommandRW<Protocol,DBAdapter>("HSET", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 4 && p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_34);

			const auto &keyN = p[1];
			const auto &subN = p[2];

			if (!hm4::Pair::isCompositeKeyValid(1, keyN, subN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			hm4::PairBufferKey bufferKey;
			auto const key = concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR, subN);

			auto const &val = p[3];
			if (!hm4::Pair::isValValid(val))
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			auto const exp  = p.size() == 5 ? from_string<uint32_t>(p[4]) : 0;

			hm4::insert(*db, key, val, exp);

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"hset",		"HSET"
		};

	};



	template<class Protocol, class DBAdapter>
	struct HMSET : BaseCommandRW<Protocol,DBAdapter>{

		HMSET() : BaseCommandRW<Protocol,DBAdapter>("HMSET", std::begin(cmd__), std::end(cmd__)){}

		// HMSET a sub1 5 sub2 6
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			auto const varg = 2;

			if (p.size() < 3 || (p.size() - varg) % 2 != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_2);

			const auto &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &subN = *itk;

				if (!hm4::Pair::isCompositeKeyValid(1, keyN, subN))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

				if (auto const &val = *std::next(itk); !hm4::Pair::isValValid(val))
					return result.set_error(ResultErrorMessages::EMPTY_VAL);
			}

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &subN = *itk;

				hm4::PairBufferKey bufferKey;
				auto const &key  = concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR, subN);
				auto const &val  = *std::next(itk);

				hm4::insert(*db, key, val);
			}

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"hmset",		"HMSET"
		};

	};



	template<class Protocol, class DBAdapter>
	struct SETNX : BaseCommandRW<Protocol,DBAdapter>{

		SETNX() : BaseCommandRW<Protocol,DBAdapter>("SETNX", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 3 && p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_23);

			// GET

			const auto &key = p[1];
			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto &val = p[2];
			if (!hm4::Pair::isValValid(val))
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			if ( hm4::getPairOK(*db, key) ){
				return result.set(false);
			}else{
				// SET

				const auto exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

				hm4::insert(*db, key, val, exp);

				return result.set(true);
			}
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"setnx",	"SETNX"
		};

	};



	template<class Protocol, class DBAdapter>
	struct SETXX : BaseCommandRW<Protocol,DBAdapter>{

		SETXX() : BaseCommandRW<Protocol,DBAdapter>("SETXX", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 3 && p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_23);

			// GET

			const auto &key = p[1];
			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto &val = p[2];
			if (!hm4::Pair::isValValid(val))
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			if (auto *it = hm4::getPairPtr(*db, key); it){
				// SET

				const auto exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

				// HINT
				const auto *hint = & *it;
				hm4::insertHintF<hm4::PairFactory::Normal>(*db, hint, key, val, exp);

				return result.set(true);
			}else{
				return result.set(false);
			}
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"setxx",	"SETXX"
		};

	};



	template<class Protocol, class DBAdapter>
	struct DEL : BaseCommandRW<Protocol,DBAdapter>{

		DEL() : BaseCommandRW<Protocol,DBAdapter>("DEL", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() < 2)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_1);

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &key = *itk;

				hm4::erase(*db, key);
			}

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"del",		"DEL"	,
			"unlink",	"UNLINK"
		};

	};



	template<class Protocol, class DBAdapter>
	struct HDEL : BaseCommandRW<Protocol,DBAdapter>{

		HDEL() : BaseCommandRW<Protocol,DBAdapter>("HDEL", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_2);

			const auto &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &subN = *itk;

				if (!hm4::Pair::isCompositeKeyValid(1, keyN, subN))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &subN = *itk;

				hm4::PairBufferKey bufferKey;
				auto const key = concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR, subN);

				hm4::erase(*db, key);
			}

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"hdel",		"HDEL"
		};

	};



	template<class Protocol, class DBAdapter>
	struct APPEND : BaseCommandRW<Protocol,DBAdapter>{

		APPEND() : BaseCommandRW<Protocol,DBAdapter>("APPEND", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			// GET

			const auto &key = p[1];
			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			std::string_view const val_new = p[2];
			if (!hm4::Pair::isValValid(val_new))
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			const auto *pair = hm4::getPairPtr(*db, key);

			std::string_view const val_old = pair ? pair->getVal() : "";

			if (!hm4::Pair::isValValid(val_new.size() + val_old.size()))
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			// HINT
			// will not work, but who knows in the future.
			const auto *hint = pair;

			hm4::insertHintV<APPEND_Factory>(*db, hint, key, pair, val_old, val_new);

			result.set(val_old.size() + val_new.size());
		}

		struct APPEND_Factory : hm4::PairFactory::IFactoryAction<1,0,APPEND_Factory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,0,APPEND_Factory>;

			std::string_view key;
			std::string_view val1;
			std::string_view val2;

			constexpr APPEND_Factory(
				std::string_view const key,
				const Pair *pair,
				std::string_view const val1,
				std::string_view const val2) :
					Base::IFactoryAction	(key, val1.size() + val2.size(), pair),
					val1				(val1		),
					val2				(val2		){}

			void action(Pair *pair) const{
				// this is already copied in:
			//	memcpy(pair->getValC() + 0,		val1.data(), val1.size());
				memcpy(pair->getValC() + val1.size(),	val2.data(), val2.size());
			}
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"append",	"APPEND"
		};

	};



	template<class Protocol, class DBAdapter>
	struct EXPIRE : BaseCommandRW<Protocol,DBAdapter>{

		EXPIRE() : BaseCommandRW<Protocol,DBAdapter>("EXPIRE", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			// GET

			const auto &key = p[1];
			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (auto *it = hm4::getPairPtr(*db, key); it){
				// SET
				auto const exp  = from_string<uint32_t>(p[2]);

				// HINT
				const auto *hint = & *it;
				hm4::insertHintF<hm4::PairFactory::Expires>(*db, hint, key, hint->getVal(), exp);

				return result.set(true);
			}else{
				return result.set(false);
			}
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"expire",	"EXPIRE"
		};

	};



	template<class Protocol, class DBAdapter>
	struct EXPIREAT : BaseCommandRW<Protocol,DBAdapter>{

		EXPIREAT() : BaseCommandRW<Protocol,DBAdapter>("EXPIREAT", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			// GET

			const auto &key = p[1];
			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (auto *it = hm4::getPairPtr(*db, key); it){
				// SET
				auto const time = from_string<uint32_t>(p[2]);
				auto const now  = mytime::now32();

				if (now >= time){
					// HINT
					const auto *hint = & *it;
					hm4::insertHintF<hm4::PairFactory::Tombstone>(*db, hint, key);

					return result.set(true);
				}else{
					// this will not overflow
					auto const exp = time - mytime::now32();

					// HINT
					const auto *hint = & *it;
					hm4::insertHintF<hm4::PairFactory::Expires>(*db, hint, key, hint->getVal(), exp);

					return result.set(true);
				}
			}else{
				return result.set(false);
			}
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"expireat",	"EXPIREAT"
		};

	};



	template<class Protocol, class DBAdapter>
	struct PERSIST : BaseCommandRW<Protocol,DBAdapter>{

		PERSIST() : BaseCommandRW<Protocol,DBAdapter>("PERSIST", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			// GET

			const auto &key = p[1];
			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (auto *it = hm4::getPairPtr(*db, key); it){
				// SET

				// pair is already checked for validity.
				if (it->isExpiresSet()){
					uint32_t const exp = 0;

					// HINT
					const auto *hint = & *it;
					hm4::insertHintF<hm4::PairFactory::Expires>(*db, hint, key, hint->getVal(), exp);
				}

				return result.set(true);
			}else{
				return result.set(false);
			}
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"persist",	"PERSIST"
		};

	};



	template<class Protocol, class DBAdapter>
	struct PERSISTDELETED : BaseCommandRW<Protocol,DBAdapter>{

		PERSISTDELETED() : BaseCommandRW<Protocol,DBAdapter>("PERSISTDELETED", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			// GET

			const auto &key = p[1];
			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (const auto *p = hm4::getPair(*db, key); p){
				// SET

				const auto *hint = p;
				hm4::insertHintF<hm4::PairFactory::Normal>(*db, hint, p->getKey(), p->getVal());
			}

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"persistdeleted",	"PERSISTDELETED"	,
			"persistexpired",	"PERSISTEXPIRED"
		};

	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "mutable";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				SET		,
				MSET		,
				SETEX		,
				HSET		,
				HMSET		,
				SETNX		,
				MSETNX		,
				MSETXX		,
				SETXX		,
				DEL		,
				HDEL		,
				APPEND		,
				EXPIRE		,
				EXPIREAT	,
				PERSIST		,
				PERSISTDELETED
			>(pack);
		}
	};

} // namespace


