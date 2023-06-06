#include "base.h"
#include "pair_vfactory.h"
#include <stdexcept>

namespace net::worker::commands::Mutable{



	template<class Protocol, class DBAdapter>
	struct SET : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3 && p.size() != 4)
				return;

			auto const &key = p[1];

			if (key.empty())
				return;

			auto const &val = p[2];
			auto const exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

			hm4::insert(*db, key, val, exp);

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"set",		"SET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MSET : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			// should be odd number arguments
			// mset a 5 b 6
			if (p.size() < 3 || p.size() % 2 == 0)
				return;

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2)
				if (const auto &key = *itk; key.empty())
					return;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &key = *itk;
				auto const &val = *std::next(itk);

				hm4::insert(*db, key, val);
			}

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mset",		"MSET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MSETNX : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			// should be odd number arguments
			// mset a 5 b 6
			if (p.size() < 3 || p.size() % 2 == 0)
				return;

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2)
				if (const auto &key = *itk; key.empty())
					return;

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
		constexpr inline static std::string_view cmd[]	= {
			"msetnx",		"MSETNX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MSETXX : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			// should be odd number arguments
			// mset a 5 b 6
			if (p.size() < 3 || p.size() % 2 == 0)
				return;

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2)
				if (const auto &key = *itk; key.empty())
					return;

			auto &container = blob.pcontainer;

			// theoretically can happen
			if (p.size() / 2 > container.capacity())
				return;

			container.clear();

			// check if any key NOT exists
			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &key = *itk;
				auto const &val = *std::next(itk);

				if (auto *it = hm4::getPairPtr(*db, key); it){
					if (const auto *hint = & *it; hm4::canInsertHint(*db, hint, val.size())){
						// HINT
						container.push_back(hint);
					}else
						container.push_back(nullptr);
				}else
					return result.set_0();
			}

			// HINT
			for(size_t i = 0; i < container.size(); ++i){
				if (const auto *hint = container[i]; hint){
					auto const &key = p[varg + i * 2 + 0];
					auto const &val = p[varg + i * 2 + 1];

					assert(key == hint->getKey());

					hm4::proceedInsertHintF<hm4::PairFactory::Normal>(*db, hint, key, val);
				}
			}

			// NORMAL
			for(size_t i = 0; i < container.size(); ++i){
				if (const auto *hint = container[i]; !hint){
					auto const &key = p[varg + i * 2 + 0];
					auto const &val = p[varg + i * 2 + 1];

					hm4::insert(*db, key, val);
				}
			}

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"msetxx",		"MSETXX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SETEX : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 4)
				return;

			auto const &key = p[1];

			if (key.empty())
				return;

			auto const &val = p[3];
			auto const exp  = from_string<uint32_t>(p[2]);

			hm4::insert(*db, key, val, exp);

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"setex",	"SETEX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HSET : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- DBAdapter::SEPARATOR.size()
						- 16;

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 4 && p.size() != 5)
				return;

			const auto &keyN = p[1];

			if (keyN.empty())
				return;

			const auto &subN = p[2];

			if (subN.empty())
				return;

			if (keyN.size() + subN.size() > MAX_KEY_SIZE)
				return;

			auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);
			auto const &val = p[3];
			auto const exp  = p.size() == 5 ? from_string<uint32_t>(p[4]) : 0;

			hm4::insert(*db, key, val, exp);

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hset",		"HSET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HMSET : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- DBAdapter::SEPARATOR.size()
						- 16;

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			// should be even number arguments
			// mset a sub1 5 sub2 6
			if (p.size() < 3 || p.size() % 2 != 0)
				return;

			const auto &keyN = p[1];

			if (keyN.empty())
				return;

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &subN = *itk;

				if (subN.empty())
					return;

				if (keyN.size() + subN.size() > MAX_KEY_SIZE)
					return;
			}

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &subN = *itk;

				auto const &key  = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);
				auto const &val  = *std::next(itk);

				hm4::insert(*db, key, val);
			}

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hmset",		"HMSET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SETNX : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3 && p.size() != 4)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;

			if ( hm4::getPairOK(*db, key) ){
				return result.set(false);
			}else{
				// SET

				const auto &val = p[2];
				const auto exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

				hm4::insert(*db, key, val, exp);

				return result.set(true);
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"setnx",	"SETNX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SETXX : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3 && p.size() != 4)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;



			if (auto *it = hm4::getPairPtr(*db, key); it){
				// SET

				const auto &val = p[2];
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
		constexpr inline static std::string_view cmd[]	= {
			"setxx",	"SETXX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct DEL : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() < 2)
				return;

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; key.empty())
					return;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &key = *itk;

				hm4::erase(*db, key);
			}

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"del",		"DEL"	,
			"unlink",	"UNLINK"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HDEL : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- DBAdapter::SEPARATOR.size()
						- 16;

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 3)
				return;

			const auto &keyN = p[1];

			if (keyN.empty())
				return;

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &subN = *itk;

				if (subN.empty())
					return;

				if (keyN.size() + subN.size() > MAX_KEY_SIZE)
					return;
			}

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &subN = *itk;

				auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);

				hm4::erase(*db, key);
			}

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hdel",		"HDEL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct GETSET : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3 && p.size() != 4)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;

			// because old_value may be overwritten,
			// we had to make a copy.

			auto it = db->find(key, std::true_type{});

			if (it != std::end(*db) && it->isOK()){
				// seamlessly send value to output buffer...
				result.set(it->getVal());
			}else{
				result.set("");
			}

			// SET

			const auto &val = p[2];
			auto const exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

			// HINT
			const auto *hint = & *it;
			hm4::insertHintF<hm4::PairFactory::Normal>(*db, hint, key, val, exp);

			// return

			return;
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"getset",	"GETSET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct GETDEL : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;



			if (auto *it = hm4::getPairPtr(*db, key); it){
				// seamlessly send value to output buffer...

				result.set(it->getVal());

				// DEL

				// HINT
				const auto *hint = & *it;
				// put tombstone
				hm4::insertHintF<hm4::PairFactory::Tombstone>(*db, hint, key);

				// return

				return;
			}else{
				return result.set("");
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"getdel",	"GETDEL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct APPEND : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;

			std::string_view const val_new = p[2];

			const auto *pair = hm4::getPairPtr(*db, key);

			std::string_view const val_old = pair ? pair->getVal() : "";

			// HINT
			// will not work, but who knows in the future.
			const auto *hint = pair;
			hm4::insertHintV<APPEND_Factory>(*db, hint, key, val_old, val_new);

			// return

			result.set(val_old.size() + val_new.size());
		}

	private:
		struct APPEND_Factory : hm4::PairFactory::IFactoryAction<0,0,APPEND_Factory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<0,0,APPEND_Factory>;

			std::string_view key;
			std::string_view val1;
			std::string_view val2;

			constexpr APPEND_Factory(
				std::string_view const key,
				std::string_view const val1,
				std::string_view const val2) :
					Base::IFactoryAction	(key, val1.size() + val2.size()),
					val1				(val1		),
					val2				(val2		){}

			void action(Pair *pair) const{
				memcpy(pair->getValC() + 0,		val1.data(), val1.size());
				memcpy(pair->getValC() + val1.size(),	val2.data(), val2.size());
			}
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"append",	"APPEND"
		};
	};



	template<class Protocol, class DBAdapter>
	struct EXPIRE : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;



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
		constexpr inline static std::string_view cmd[]	= {
			"expire",	"EXPIRE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct PERSIST : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;



			if (auto *it = hm4::getPairPtr(*db, key); it){
				// SET

				if (it->getTTL() > 0){
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
		constexpr inline static std::string_view cmd[]	= {
			"persist",	"PERSIST"
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
				GETSET		,
				GETDEL		,
				APPEND		,
				EXPIRE		,
				PERSIST
			>(pack);
		}
	};


} // namespace


