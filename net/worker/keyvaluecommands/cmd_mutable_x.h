#include "base.h"
#include "mystring.h"
#include "logger.h"

#include <algorithm>	// std::clamp

namespace net::worker::commands::MutableX{



	namespace getx_impl_{

		constexpr static uint32_t ITERATIONS_PROCESS_X	=  OutputBlob::ContainerSize;
		constexpr static uint32_t PASSES_PROCESS_X	= 3;

		using ContainerX = typename OutputBlob::PairContainer;



		template<class Predicate, class List>
		std::string_view scanKeysAndProcessInPlace_(Predicate &p, List &list, std::string_view prefix, std::string_view key, ContainerX &container){
			uint32_t iterations = 0;

			for(auto it = list.find(key, std::false_type{});it != std::end(list);++it){
				auto const key = it->getKey();

				if (! same_prefix(prefix, key))
					return {};

				if (++iterations > ITERATIONS_PROCESS_X){
					log__<LogLevel::NOTICE>("ProcessX", "iterations", iterations, key);
					return key;
				}

				if (! it->isValid(std::true_type{}))
					continue;

				// HINT !!!

				if (const auto *hint = & *it; hm4::canInsertHint(list, hint)){
					p.processHint(list, hint);
				}else{
					container.emplace_back(hint);
				}
			}

			return {};
		}





		template<class List, class Result>
		void scanForLastKey_(List &list, std::string_view prefix, std::string_view key, Result &result){
			uint32_t iterations = 0;

			for(auto it = list.find(key, std::false_type{});it != std::end(list);++it){
				auto const key = it->getKey();

				if (! same_prefix(prefix, key))
					return result.set();

				if (++iterations > ITERATIONS_PROCESS_X){
					log__<LogLevel::NOTICE>("ProcessX", "iterations", iterations, key);
					return result.set(key);
				}

				if (! it->isValid(std::true_type{}))
					continue;

				return result.set(key);
			}

			return result.set();
		}





		template<class Predicate, class List, class Result>
		void process_x(Predicate &p, List &list, std::string_view prefix, std::string_view key, Result &result, ContainerX &container){
			container.clear();

			uint8_t check_passes = 0;

		// label for goto
		start:
			std::string_view string_key = scanKeysAndProcessInPlace_(p, list, prefix, key, container);

			for(auto const &x : container){
				auto const s1 = list.mutable_size();

				p.process(list, x);

				auto const s2 = list.mutable_size();

				if (s2 < s1){
					// something hapenned.
					// list just flushed.
					// the container contains junk now.
					++check_passes;

					log__<LogLevel::WARNING>("ProcessX", "Restart because of table flush", check_passes);

					if (check_passes < PASSES_PROCESS_X)
						goto start;
					else
						return scanForLastKey_(list, prefix, key, result);
				}
			}

			return result.set(string_key);
		}



		template<class DBAdapter>
		struct DeletePredicate{
			using List = typename DBAdapter::List;

			static void process(List &list, const hm4::Pair *hint){
				// put tombstone
				hm4::insert(list, hint->getKey());
			}

			static void processHint(List &list, const hm4::Pair *hint){
				// put tombstone
				hm4::insertHintF<hm4::PairFactory::Tombstone>(list, hint, hint->getKey());
			}
		};

		template<class DBAdapter>
		struct PersistPredicate{
			using List = typename DBAdapter::List;

			constexpr static uint32_t expires = 0;

			static void process(List &list, const hm4::Pair *hint){
				hm4::insert(list, hint->getKey(), hint->getVal(), expires);
			}

			static void processHint(List &list, const hm4::Pair *hint){
				hm4::insertHintF<hm4::PairFactory::NormalExpiresOnly>(list, hint, hint->getKey(), hint->getVal(), expires);
			}
		};

		template<class DBAdapter>
		struct ExpirePredicate{
			using List = typename DBAdapter::List;

			uint32_t expires;

			void process(List &list, const hm4::Pair *hint) const{
				hm4::insert(list, hint->getKey(), hint->getVal(), expires);
			}

			void processHint(List &list, const hm4::Pair *hint) const{
				hm4::insertHintF<hm4::PairFactory::NormalExpiresOnly>(list, hint, hint->getKey(), hint->getVal(), expires);
			}
		};
	} // namespace



	// =======================



	template<class Protocol, class DBAdapter>
	struct DELX : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 3)
				return;

			auto const &key    = p[1];
			auto const &prefix = p[2];

			if (prefix.empty())
				return;

			using namespace getx_impl_;

			DeletePredicate<DBAdapter> pred;
			return process_x(pred, *db, prefix, key, result, blob.pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"delx",		"DELX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct PERSISTX : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 3)
				return;

			auto const &key		= p[1];
			auto const &prefix	= p[2];

			if (prefix.empty())
				return;

			using namespace getx_impl_;

			PersistPredicate<DBAdapter> pred;
			return process_x(pred, *db, prefix, key, result, blob.pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"persistx",		"PERSISTX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct EXPIREX : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 4)
				return;

			auto const &key		= p[1];
			auto const exp		= from_string<uint32_t>(p[2]);
			auto const &prefix	= p[3];

			if (prefix.empty())
				return;

			using namespace getx_impl_;

			ExpirePredicate<DBAdapter> pred{exp};
			return process_x(pred, *db, prefix, key, result, blob.pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"expirex",		"EXPIREX"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "getx";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				DELX		,
				PERSISTX	,
				EXPIREX
			>(pack);
		}
	};



} // namespace

