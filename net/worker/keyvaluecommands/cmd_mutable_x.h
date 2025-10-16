#include "base.h"
#include "mystring.h"
#include "logger.h"

#include <algorithm>	// std::clamp

#include "shared_stoppredicate.h"

#include "ilist/txguard.h"

namespace net::worker::commands::MutableX{
	using PContainerX = typename OutputBlob::PairContainer;
	using ContainerX  = typename OutputBlob::Container;
	using BufferX     = typename OutputBlob::BufferKContainer;

	namespace mutablex_impl_{

		using namespace net::worker::shared::stop_predicate;

		constexpr static uint32_t ITERATIONS_PROCESS_X	= OutputBlob::ContainerSize;
		constexpr static uint32_t PASSES_PROCESS_X	= 3;

		namespace{
			template<class Predicate, class StopPredicate, class List>
			std::string_view scanKeysAndProcessInPlace_(Predicate p, StopPredicate stop, List &list, std::string_view key, PContainerX &pcontainer){
				static_assert(!Predicate::BIG_BUFFER);

				pcontainer.clear();

				uint32_t iterations = 0;

				for(auto it = list.find(key); it != std::end(list); ++it){
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
						pcontainer.emplace_back(hint);
					}
				}

				return {};
			}

			template<class StopPredicate, class List, class Result>
			void scanForLastKey_(StopPredicate stop, List &list, std::string_view key, Result &result){
				uint32_t iterations = 0;

				for(auto it = list.find(key); it != std::end(list); ++it){
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
			void process_x_(Predicate p, StopPredicate stop, List &list, std::string_view key, Result &result, PContainerX &pcontainer){
				static_assert(!Predicate::BIG_BUFFER);

				[[maybe_unused]]
				hm4::TXGuard guard{ list };

				uint8_t check_passes = 0;

			// label for goto
			start:
				std::string_view last_key = scanKeysAndProcessInPlace_(p, stop, list, key, pcontainer);

				// update by insert

				for(auto const &x : pcontainer){
					auto const v1 = list.mutable_version();

					p.process(list, x);

					auto const v2 = list.mutable_version();

					if (v1 != v2){
						// The list version is different.
						// It means the list just flushed and
						// the pcontainer contains junk now.
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





			template<class Predicate, class StopPredicate, class List>
			std::string_view scanKeysAndProcessInPlace_(Predicate p, StopPredicate stop, List &list, std::string_view key, ContainerX &container, BufferX &bcontainer){
				static_assert(Predicate::BIG_BUFFER);

				container.clear();
				bcontainer.clear();

				uint32_t iterations = 0;

				for(auto it = list.find(key); it != std::end(list); ++it){
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
						// now we have lots of memory,
						// so we can make a copy

						bcontainer.emplace_back();

						auto const key = concatenateBuffer(bcontainer.back(), hint->getKey());

						container.emplace_back(key);
					}
				}

				return {};
			}

			template<bool ResultAsHash, class Predicate, class StopPredicate, class List, class Result>
			void process_x_(Predicate p, StopPredicate stop, List &list, std::string_view key, Result &result, ContainerX &container, BufferX &bcontainer){
				static_assert(Predicate::BIG_BUFFER);

				[[maybe_unused]]
				hm4::TXGuard guard{ list };

				// update in place and populate the containers

				std::string_view const last_key = scanKeysAndProcessInPlace_(p, stop, list, key, container, bcontainer);

				// update by insert, the container contains stable pointers
				// we can update everything very easy, because is already checked

				for(auto const &x : container)
					p.process(list, x);

				if constexpr(ResultAsHash)
					return result.set_1();
				else
					return result.set(last_key);
			}



			template<class Predicate, class StopPredicate, class List, class Result, typename... Containers>
			void process_x(Predicate p, StopPredicate stop, List &list, std::string_view key, Result &result, Containers &&...containers){
				return process_x_<0>(p, stop, list, key, result, std::forward<Containers>(containers)...);
			}

			template<class Predicate, class List, class Result, typename... Containers>
			void process_h(Predicate p, List &list, std::string_view prefix, Result &result, Containers &&...containers){
				StopPrefixPredicate stop{ prefix };
				return process_x_<1>(p, stop, list, prefix, result, std::forward<Containers>(containers)...);
			}



			template<class DBAdapter>
			struct DeletePredicate{
				using List = typename DBAdapter::List;

				constexpr static bool BIG_BUFFER = true;

				static void process(List &list, std::string_view key){
					// erase
					hm4::erase(list, key);
				}

				static void processHint(List &list, const hm4::Pair *hint){
					// put tombstone
					hm4::insertHintF<hm4::PairFactory::Tombstone>(list, hint, hint->getKey());
				}
			};

			template<class DBAdapter>
			struct PersistPredicate{
				using List = typename DBAdapter::List;

				constexpr static bool BIG_BUFFER = false;

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

				constexpr static bool BIG_BUFFER = false;

				uint32_t expires;

				void process(List &list, const hm4::Pair *hint) const{
					hm4::insert(list, hint->getKey(), hint->getVal(), expires);
				}

				void processHint(List &list, const hm4::Pair *hint) const{
					hm4::insertHintF<hm4::PairFactory::Expires>(list, hint, hint->getKey(), hint->getVal(), expires);
				}
			};

			template<class DBAdapter>
			struct ExpireAtPredicate{
				using List = typename DBAdapter::List;

				constexpr static bool BIG_BUFFER = false;

				uint32_t time;

				void process(List &list, const hm4::Pair *hint) const{
					if (auto const now = mytime::now32(); now >= time){
						hm4::erase(list, hint->getKey());
					}else{
						// this will not overflow
						auto const expires = time - mytime::now32();

						hm4::insert(list, hint->getKey(), hint->getVal(), expires);
					}
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
		} // namespace
	} // namespace mutablex_impl_



	// =======================



	template<class Protocol, class DBAdapter>
	struct XNDEL : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &key		= p[1];
			auto const &prefix	= p[2];

			if (prefix.empty())
				return result.set_error(ResultErrorMessages::EMPTY_PREFIX);

			using namespace mutablex_impl_;

			DeletePredicate<DBAdapter>	pred;
			StopPrefixPredicate		stop{ prefix };
			auto &container  = blob.construct<ContainerX>();
			auto &bcontainer = blob.construct<BufferX>();
			return process_x(pred, stop, *db, key, result, container, bcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xndel",	"XNDEL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XRDEL : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &key		= p[1];
			auto const &keyEnd	= p[2];

			if (!hm4::Pair::isKeyValid(keyEnd))
				return result.set_error(ResultErrorMessages::EMPTY_ENDCOND);

			using namespace mutablex_impl_;

			DeletePredicate<DBAdapter>	pred;
			StopRangePredicate		stop{ keyEnd };
			auto &container  = blob.construct<ContainerX>();
			auto &bcontainer = blob.construct<BufferX>();
			return process_x(pred, stop, *db, key, result, container, bcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xrdel",	"XRDEL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HDELALL : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			auto const &keyN	= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!hm4::Pair::isKeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			hm4::PairBufferKey bufferKey;
			auto const &prefix	= concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR);

			using namespace mutablex_impl_;

			DeletePredicate<DBAdapter>	pred;
			auto &container  = blob.construct<ContainerX>();
			auto &bcontainer = blob.construct<BufferX>();
			return process_h(pred, *db, prefix, result, container, bcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hdelall",		"HDELALL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XNPERSIST : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &key		= p[1];
			auto const &prefix	= p[2];

			if (prefix.empty())
				return result.set_error(ResultErrorMessages::EMPTY_PREFIX);

			using namespace mutablex_impl_;

			PersistPredicate<DBAdapter>	pred;
			StopPrefixPredicate		stop{ prefix };
			auto &pcontainer = blob.construct<PContainerX>();
			return process_x(pred, stop, *db, key, result, pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xnpersist",		"XNPERSIST"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XRPERSIST : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &key		= p[1];
			auto const &keyEnd	= p[2];

			if (!hm4::Pair::isKeyValid(keyEnd))
				return result.set_error(ResultErrorMessages::EMPTY_ENDCOND);

			using namespace mutablex_impl_;

			PersistPredicate<DBAdapter>	pred;
			StopRangePredicate		stop{ keyEnd };
			auto &pcontainer = blob.construct<PContainerX>();
			return process_x(pred, stop, *db, key, result, pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xrpersist",		"XRPERSIST"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HPERSISTALL : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			auto const &keyN	= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!hm4::Pair::isKeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			hm4::PairBufferKey bufferKey;
			auto const &prefix	= concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR);

			using namespace mutablex_impl_;

			PersistPredicate<DBAdapter>	pred;
			auto &pcontainer = blob.construct<PContainerX>();
			return process_h(pred, *db, prefix, result, pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hpersistall",		"HPERSISTALL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XNEXPIRE : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &key		= p[1];
			auto const exp		= from_string<uint32_t>(p[2]);
			auto const &prefix	= p[3];

			if (prefix.empty())
				return result.set_error(ResultErrorMessages::EMPTY_PREFIX);

			using namespace mutablex_impl_;

			ExpirePredicate<DBAdapter>	pred{exp};
			StopPrefixPredicate		stop{ prefix };
			auto &pcontainer = blob.construct<PContainerX>();
			return process_x(pred, stop, *db, key, result, pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xnexpire",		"XNEXPIRE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XREXPIRE : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &key		= p[1];
			auto const exp		= from_string<uint32_t>(p[2]);
			auto const &keyEnd	= p[3];

			if (!hm4::Pair::isKeyValid(keyEnd))
				return result.set_error(ResultErrorMessages::EMPTY_PREFIX);

			using namespace mutablex_impl_;

			ExpirePredicate<DBAdapter>	pred{exp};
			StopRangePredicate		stop{ keyEnd };
			auto &pcontainer = blob.construct<PContainerX>();
			return process_x(pred, stop, *db, key, result, pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xrexpire",		"XREXPIRE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HEXPIREALL : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN	= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!hm4::Pair::isKeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			hm4::PairBufferKey bufferKey;
			auto const &prefix	= concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR);

			auto const exp		= from_string<uint32_t>(p[2]);

			using namespace mutablex_impl_;

			ExpirePredicate<DBAdapter>	pred{exp};
			auto &pcontainer = blob.construct<PContainerX>();
			return process_h(pred, *db, prefix, result, pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hexpireall",		"HEXPIREALL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XNEXPIREAT : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &key		= p[1];
			auto const time		= from_string<uint32_t>(p[2]);
			auto const &prefix	= p[3];

			if (prefix.empty())
				return result.set_error(ResultErrorMessages::EMPTY_PREFIX);

			using namespace mutablex_impl_;

			ExpireAtPredicate<DBAdapter>	pred{time};
			StopPrefixPredicate		stop{ prefix };
			auto &pcontainer = blob.construct<PContainerX>();
			return process_x(pred, stop, *db, key, result, pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xnexpireat",		"XNEXPIREAT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XREXPIREAT : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &key		= p[1];
			auto const time		= from_string<uint32_t>(p[2]);
			auto const &keyEnd	= p[3];

			if (!hm4::Pair::isKeyValid(keyEnd))
				return result.set_error(ResultErrorMessages::EMPTY_ENDCOND);

			using namespace mutablex_impl_;

			ExpireAtPredicate<DBAdapter>	pred{time};
			StopRangePredicate		stop{ keyEnd };
			auto &pcontainer = blob.construct<PContainerX>();
			return process_x(pred, stop, *db, key, result, pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xrexpireay",		"XREXPIREAT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HEXPIREATALL : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN	= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!hm4::Pair::isKeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			hm4::PairBufferKey bufferKey;
			auto const &prefix	= concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR);

			auto const time		= from_string<uint32_t>(p[2]);

			using namespace mutablex_impl_;

			ExpireAtPredicate<DBAdapter>	pred{time};
			auto &pcontainer = blob.construct<PContainerX>();
			return process_h(pred, *db, prefix, result, pcontainer);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hexpireatall",		"HEXPIREATALL"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "mutable_x";

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
				HEXPIREALL	,

				XNEXPIREAT	,
				XREXPIREAT	,
				HEXPIREATALL
			>(pack);
		}
	};



} // namespace

