#include "mytime.h"

namespace net::worker::commands::MutableGET{

	namespace mutable_get_impl_{
		namespace{

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

		} // namespace
	} // namespace mutable_get_impl_



	template<class Protocol, class DBAdapter>
	struct GETSET : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return;

			const auto &key = p[1];
			if (!hm4::Pair::isKeyValid(key))
				return;

			const auto &val = p[2];
			if (!hm4::Pair::isValValid(val))
				return;

			using namespace mutable_get_impl_;

			SetPredicate<DBAdapter>	pred{ key, val };

			return processGet(pred, *db, key, result);
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

			const auto &key = p[1];
			if (!hm4::Pair::isKeyValid(key))
				return;

			using namespace mutable_get_impl_;

			DeletePredicate<DBAdapter>	pred;

			return processGet(pred, *db, key, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"getdel",	"GETDEL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct GETEX : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return;

			const auto &key = p[1];
			if (!hm4::Pair::isKeyValid(key))
				return;

			auto const exp		= from_string<uint32_t>(p[2]);

			using namespace mutable_get_impl_;

			ExpirePredicate<DBAdapter>	pred{ exp };

			return processGet(pred, *db, key, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"getex",	"GETEX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct GETEXAT : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return;

			const auto &key = p[1];
			if (!hm4::Pair::isKeyValid(key))
				return;

			auto const time		= from_string<uint32_t>(p[2]);

			using namespace mutable_get_impl_;

			ExpireAtPredicate<DBAdapter>	pred{ time };

			return processGet(pred, *db, key, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"getexat",	"GETEXAT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct GETPERSIST : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return;

			const auto &key = p[1];
			if (!hm4::Pair::isKeyValid(key))
				return;

			using namespace mutable_get_impl_;

			PersistPredicate<DBAdapter>	pred;

			return processGet(pred, *db, key, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"getpersist",	"GETPERSIST"
		};
	};







	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "mutable";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				GETSET		,
				GETDEL		,
				GETEX		,
				GETEXAT		,
				GETPERSIST
			>(pack);
		}
	};

} // namespace

