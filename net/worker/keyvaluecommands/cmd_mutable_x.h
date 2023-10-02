#include "base.h"
#include "mystring.h"
#include "logger.h"

#include <algorithm>	// std::clamp

namespace net::worker::commands::MutableX{
	namespace mutablex_impl_{
		namespace{
			constexpr static uint32_t ITERATIONS_PROCESS_X	= OutputBlob::ContainerSize;
			constexpr static uint32_t PASSES_PROCESS_X	= 3;

			using ContainerX = typename OutputBlob::PairContainer;



			template<class Predicate, class StopPredicate, class List>
			std::string_view scanKeysAndProcessInPlace_(Predicate p, StopPredicate stop, List &list, std::string_view key, ContainerX &container){
				uint32_t iterations = 0;

				for(auto it = list.find(key, std::false_type{}); it != std::end(list); ++it){
					auto const key = it->getKey();

					if (stop(key))
						return {};

					if (++iterations > ITERATIONS_PROCESS_X){
						logger<Logger::DEBUG>() << "ProcessX" << "iterations" << iterations << key;
						return key;
					}

					if (! it->isOK())
						continue;

					// HINT !!!

					if (const auto *hint = & *it; hm4::canInsertHintList(list, hint)){
						p.processHint(list, hint);
					}else{
						container.emplace_back(hint);
					}
				}

				return {};
			}





			template<class StopPredicate, class List, class Result>
			void scanForLastKey_(StopPredicate stop, List &list, std::string_view key, Result &result){
				uint32_t iterations = 0;

				for(auto it = list.find(key, std::false_type{}); it != std::end(list); ++it){
					auto const key = it->getKey();

					if (stop(key))
						return result.set();

					if (++iterations > ITERATIONS_PROCESS_X){
						logger<Logger::DEBUG>() << "ProcessX" << "iterations" << iterations << key;
						return result.set(key);
					}

					if (! it->isOK())
						continue;

					return result.set(key);
				}

				return result.set();
			}





			template<bool ResultAsHash, class Predicate, class StopPredicate, class List, class Result>
			void process_x_(Predicate p, StopPredicate stop, List &list, std::string_view key, Result &result, ContainerX &container){
				container.clear();

				uint8_t check_passes = 0;

			// label for goto
			start:
				std::string_view last_key = scanKeysAndProcessInPlace_(p, stop, list, key, container);

				// update by insert

				for(auto const &x : container){
					auto const s1 = list.mutable_list().size();

					p.process(list, x);

					auto const s2 = list.mutable_list().size();

					if (s2 < s1){
						// something hapenned.
						// list just flushed.
						// the container contains junk now.
						++check_passes;

						logger<Logger::NOTICE>() << "ProcessX" << "Restart because of table flush" << check_passes;

						if (check_passes < PASSES_PROCESS_X)
							goto start;

						if constexpr(ResultAsHash)
							return result.set_0();
						else
							return scanForLastKey_(stop, list, key, result);
					}
				}

				if constexpr(ResultAsHash)
					return result.set_1();
				else
					return result.set(last_key);
			}



			template<class Predicate, class StopPredicate, class List, class Result>
			void process_x(Predicate p, StopPredicate stop, List &list, std::string_view key, Result &result, ContainerX &container){
				return process_x_<0>(p, stop, list, key, result, container);
			}



			// moved for clang
			// making it class, makes later code prettier.
			struct StopPrefixPredicate{
				std::string_view prefix;

				bool operator()(std::string_view key) const{
					return ! same_prefix(prefix, key);
				}
			};

			struct StopRangePredicate{
				std::string_view end;

				constexpr bool operator()(std::string_view key) const{
					return end < key;
				}
			};




			template<class Predicate, class List, class Result>
			void process_h(Predicate p, List &list, std::string_view prefix, Result &result, ContainerX &container){
				StopPrefixPredicate stop{ prefix };
				return process_x_<1>(p, stop, list, prefix, result, container);
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
					hm4::insertHintF<hm4::PairFactory::Expires>(list, hint, hint->getKey(), hint->getVal(), expires);
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
					hm4::insertHintF<hm4::PairFactory::Expires>(list, hint, hint->getKey(), hint->getVal(), expires);
				}
			};
		} // namespace
	} // namespace mutablex_impl_



	// =======================



	template<class Protocol, class DBAdapter>
	struct XNDEL : BaseRW<Protocol,DBAdapter>{
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

			using namespace mutablex_impl_;

			DeletePredicate<DBAdapter>	pred;
			StopPrefixPredicate		stop{ prefix };
			return process_x(pred, stop, *db, key, result, blob.pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xndel",	"XNDEL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XRDEL : BaseRW<Protocol,DBAdapter>{
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
			auto const &end		= p[2];

			if (end.empty())
				return;

			using namespace mutablex_impl_;

			DeletePredicate<DBAdapter>	pred;
			StopRangePredicate		stop{ end };
			return process_x(pred, stop, *db, key, result, blob.pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xrdel",	"XRDEL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HDELALL : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 2)
				return;

			auto const &keyN	= p[1];

			if (keyN.empty())
				return;

			auto const &prefix	= concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR);

			using namespace mutablex_impl_;

			DeletePredicate<DBAdapter>	pred;
			return process_h(pred, *db, prefix, result, blob.pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hdelall",		"HDELALL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XNPERSIST : BaseRW<Protocol,DBAdapter>{
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

			using namespace mutablex_impl_;

			PersistPredicate<DBAdapter>	pred;
			StopPrefixPredicate		stop{ prefix };
			return process_x(pred, stop, *db, key, result, blob.pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xnpersist",		"XNPERSIST"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XRPERSIST : BaseRW<Protocol,DBAdapter>{
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
			auto const &end		= p[2];

			if (end.empty())
				return;

			using namespace mutablex_impl_;

			PersistPredicate<DBAdapter>	pred;
			StopRangePredicate		stop{ end };
			return process_x(pred, stop, *db, key, result, blob.pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xrpersist",		"XRPERSIST"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HPERSISTALL : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 2)
				return;

			auto const &keyN	= p[1];

			if (keyN.empty())
				return;

			auto const &prefix	= concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR);

			using namespace mutablex_impl_;

			PersistPredicate<DBAdapter>	pred;
			return process_h(pred, *db, prefix, result, blob.pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hpersistall",		"HPERSISTALL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XNEXPIRE : BaseRW<Protocol,DBAdapter>{
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

			using namespace mutablex_impl_;

			ExpirePredicate<DBAdapter>	pred{exp};
			StopPrefixPredicate		stop{ prefix };
			return process_x(pred, stop, *db, key, result, blob.pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xnexpire",		"XNEXPIRE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XREXPIRE : BaseRW<Protocol,DBAdapter>{
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
			auto const &end		= p[3];

			if (end.empty())
				return;

			using namespace mutablex_impl_;

			ExpirePredicate<DBAdapter>	pred{exp};
			StopRangePredicate		stop{ end };
			return process_x(pred, stop, *db, key, result, blob.pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xrexpire",		"XREXPIRE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HEXPIREALL : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 3)
				return;

			auto const &keyN	= p[1];

			if (keyN.empty())
				return;

			auto const &prefix	= concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR);

			auto const exp		= from_string<uint32_t>(p[2]);

			using namespace mutablex_impl_;

			ExpirePredicate<DBAdapter>	pred{exp};
			return process_h(pred, *db, prefix, result, blob.pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hexpireall",		"HEXPIREALL"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "getx";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				XNDEL		,
				XRDEL		,
				HDELALL		,

				XNPERSIST	,
				XRPERSIST	,
				HPERSISTALL	,

				XNEXPIRE	,
				XREXPIRE	,
				HEXPIREALL
			>(pack);
		}
	};



} // namespace

