#include "mytime.h"

namespace net::worker::commands::MutableGET{

	namespace impl_{

		template<class DBAdapter>
		struct SetPredicate{
			using List = typename DBAdapter::List;

			std::string_view key;
			std::string_view val;

			void process(List &list){
				hm4::insert(list, key, val);
			}

			void processHint(List &list, const hm4::Pair *hint){
				assert(key ==  hint->getKey());
				hm4::insertHintF<hm4::PairFactory::Normal>(list, hint, hint->getKey(), val);
			}
		};

		template<class DBAdapter>
		struct DeletePredicate{
			using List = typename DBAdapter::List;

			static void process(List &){
			}

			static void processHint(List &list, const hm4::Pair *hint){
				hm4::insertHintF<hm4::PairFactory::Tombstone>(list, hint, hint->getKey());
			}
		};

		template<class DBAdapter>
		struct PersistPredicate{
			using List = typename DBAdapter::List;

			constexpr static uint32_t expires = 0;

			static void process(List &){
			}

			static void processHint(List &list, const hm4::Pair *hint){
				hm4::insertHintF<hm4::PairFactory::Expires>(list, hint, hint->getKey(), hint->getVal(), expires);
			}
		};

		template<class DBAdapter>
		struct ExpirePredicate{
			using List = typename DBAdapter::List;

			uint32_t expires;

			static void process(List &){
			}

			void processHint(List &list, const hm4::Pair *hint) const{
				hm4::insertHintF<hm4::PairFactory::Expires>(list, hint, hint->getKey(), hint->getVal(), expires);
			}
		};

		template<class DBAdapter>
		struct ExpireAtPredicate{
			using List = typename DBAdapter::List;

			uint32_t time;

			static void process(List &){
			}

			void processHint(List &list, const hm4::Pair *hint) const{
				if (auto const now = mytime::now32(); now >= time){
					hm4::insertHintF<hm4::PairFactory::Tombstone>(list, hint, hint->getKey());
				}else{
					// this will not overflow
					auto const expires = time - mytime::now32();

					hm4::insertHintF<hm4::PairFactory::Expires>(list, hint, hint->getKey(), hint->getVal(), expires);
				}
			}
		};



		template<class Predicate, class List, class Result>
		void processGet(Predicate p, List &list, std::string_view key, Result &result){
			if (auto *it = hm4::getPairPtr(list, key); it){
				result.set(it->getVal());

				return p.processHint(list, it);
			}else{
				result.set("");

				return p.process(list);
			}
		}

	} // namespace impl_



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct GETSET : BaseCommandRW<Protocol,MemoryManager,DBAdapter>{

		GETSET() : BaseCommandRW<Protocol,MemoryManager,DBAdapter>("GETSET", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			const auto &key = p[1];
			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto &val = p[2];
			if (!hm4::Pair::isValValid(val))
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			using namespace impl_;

			SetPredicate<DBAdapter>	pred{ key, val };

			return processGet(pred, *db, key, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"getset",	"GETSET"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct GETDEL : BaseCommandRW<Protocol,MemoryManager,DBAdapter>{

		GETDEL() : BaseCommandRW<Protocol,MemoryManager,DBAdapter>("GETDEL", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			const auto &key = p[1];
			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			DeletePredicate<DBAdapter>	pred;

			return processGet(pred, *db, key, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"getdel",	"GETDEL"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct GETEX : BaseCommandRW<Protocol,MemoryManager,DBAdapter>{

		GETEX() : BaseCommandRW<Protocol,MemoryManager,DBAdapter>("GETEX", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			const auto &key = p[1];
			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const exp		= from_string<uint32_t>(p[2]);

			using namespace impl_;

			ExpirePredicate<DBAdapter>	pred{ exp };

			return processGet(pred, *db, key, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"getex",	"GETEX"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct GETEXAT : BaseCommandRW<Protocol,MemoryManager,DBAdapter>{

		GETEXAT() : BaseCommandRW<Protocol,MemoryManager,DBAdapter>("GETEXAT", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			const auto &key = p[1];
			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const time		= from_string<uint32_t>(p[2]);

			using namespace impl_;

			ExpireAtPredicate<DBAdapter>	pred{ time };

			return processGet(pred, *db, key, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"getexat",	"GETEXAT"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct GETPERSIST : BaseCommandRW<Protocol,MemoryManager,DBAdapter>{

		GETPERSIST() : BaseCommandRW<Protocol,MemoryManager,DBAdapter>("GETPERSIST", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			const auto &key = p[1];
			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			PersistPredicate<DBAdapter>	pred;

			return processGet(pred, *db, key, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"getpersist",	"GETPERSIST"
		};

	};







	template<typename Protocol, typename MemoryManager, typename DBAdapter, typename RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "mutable";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, MemoryManager, DBAdapter, RegisterPack,
				GETSET		,
				GETDEL		,
				GETEX		,
				GETEXAT		,
				GETPERSIST
			>(pack);
		}
	};

} // namespace

