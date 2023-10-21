#include "base.h"
#include "mystring.h"
#include "logger.h"

#include <algorithm>	// std::clamp

#include "shared_stoppredicate.h"

namespace net::worker::commands::ImmutableX{
	namespace immutablex_impl_{

		using namespace net::worker::shared::stop_predicate;

		namespace {

			constexpr static uint32_t MIN_ITERATIONS	= 10;
			constexpr static uint32_t ITERATIONS		= (OutputBlob::ContainerSize - 1) / 2;



			auto myClamp(std::string_view p){
				// using uint64_t from the user, allow more user-friendly behavour.
				// suppose he / she enters 1'000'000'000.
				// because this value is great than max uint32_t,
				// the converted value will go to 0, then to MIN.

				auto const a = from_string<uint64_t>(p);

				return static_cast<uint32_t>(
					std::clamp<uint64_t>(a, MIN_ITERATIONS, ITERATIONS)
				);
			};



			template<class DBAdapter>
			constexpr static std::size_t MAX_HKEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
							- DBAdapter::SEPARATOR.size()
							- 16;



			enum class AccumulateOutput{
				KEYS,
				KEYS_WITH_TAIL,
				VALS,
				BOTH,
				BOTH_WITH_TAIL
			};

			template<class It>
			uint32_t countResultsH(std::string_view const prefix, It it, It eit){
				uint32_t iterations	= 0;
				uint32_t results	= 0;

				StopPrefixPredicate stop{ prefix };

				for(;it != eit;++it){
					auto const &key = it->getKey();

					if (++iterations > ITERATIONS)
						break;

					if (stop(key))
						break;

					if (! it->isOK())
						continue;

					++results;
				}

				return results;
			}

			template<AccumulateOutput Out, class StopPredicate, class It, class Container, class ProjectionKey>
			void accumulateResults_(uint32_t const maxResults, StopPredicate stop, It it, It eit, Container &container, ProjectionKey projKey){
				uint32_t iterations	= 0;
				uint32_t results	= 0;

				container.clear();

				// capture & instead of &container to silence clang warning.
				auto tail = [&](auto const &pkey){
					if constexpr(Out == AccumulateOutput::BOTH_WITH_TAIL || Out == AccumulateOutput::KEYS_WITH_TAIL)
						container.emplace_back(pkey);
				};

				for(;it != eit;++it){
					auto const &key = it->getKey();

					auto pkey = projKey(key);

					if (++iterations > ITERATIONS)
						return tail(pkey);

					if (stop(key))
						return; // no tail

					if (! it->isOK())
						continue;

					if (++results > maxResults)
						return tail(pkey);

					auto const &val = it->getVal();

					if constexpr(Out == AccumulateOutput::BOTH_WITH_TAIL || Out == AccumulateOutput::BOTH || Out == AccumulateOutput::KEYS || Out == AccumulateOutput::KEYS_WITH_TAIL)
						container.emplace_back(pkey);

					if constexpr(Out == AccumulateOutput::BOTH_WITH_TAIL || Out == AccumulateOutput::BOTH || Out == AccumulateOutput::VALS)
						container.emplace_back(val);

					if constexpr(Out == AccumulateOutput::KEYS_WITH_TAIL)
						container.emplace_back("1");
				}
			}

			template<AccumulateOutput Out, class StopPredicate, class It, class Container>
			void accumulateResultsX(uint32_t const maxResults, StopPredicate stop, It it, It eit, Container &container){
				auto proj = [](std::string_view x){
					return x;
				};

				return accumulateResults_<Out>(maxResults, stop, it, eit, container, proj);
			}

			template<AccumulateOutput Out, class StopPredicate, class It, class Container>
			void accumulateResultsXKeys(uint32_t const maxResults, StopPredicate stop, It it, It eit, Container &container){
				auto proj = [](std::string_view x){
					return x;
				};

				return accumulateResults_<Out>(maxResults, stop, it, eit, container, proj);
			}

			template<AccumulateOutput Out, class It, class Container>
			void accumulateResultsH(uint32_t const maxResults, std::string_view const prefix, It it, It eit, Container &container){
				auto const prefix_size = prefix.size();

				auto proj = [prefix_size](std::string_view x) -> std::string_view{
					if (prefix_size <= x.size())
						return x.substr(prefix_size);
					else
						return x;
				};

				StopPrefixPredicate stop{ prefix };

				return accumulateResults_<Out>(maxResults, stop, it, eit, container, proj);
			}

		} // namespace

	} // namespace immutablex_impl_



	template<class Protocol, class DBAdapter>
	struct XNGET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 4)
				return;



			using namespace immutablex_impl_;

			static_assert(OutputBlob::ContainerSize >= 2 * ITERATIONS + 1);



			auto const key    = p[1];
			auto const count  = myClamp(p[2]);
			auto const prefix = p[3];

			if (prefix.empty())
				return;

			StopPrefixPredicate stop{ prefix };

			accumulateResultsX<AccumulateOutput::BOTH_WITH_TAIL>(
				count					,
				stop					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				blob.container
			);

			return result.set_container(blob.container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xnget",	"XNGET",
			"getx",		"GETX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XRGET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 4)
				return;



			using namespace immutablex_impl_;

			static_assert(OutputBlob::ContainerSize >= 2 * ITERATIONS + 1);



			auto const key   = p[1];
			auto const count = myClamp(p[2]);
			auto const end   = p[3];

			if (end.empty())
				return;

			StopRangePredicate stop{ end };

			accumulateResultsX<AccumulateOutput::BOTH_WITH_TAIL>(
				count					,
				stop					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				blob.container
			);

			return result.set_container(blob.container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xrget",	"XRGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XUGET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 3)
				return;



			using namespace immutablex_impl_;

			static_assert(OutputBlob::ContainerSize >= 2 * ITERATIONS + 1);



			auto const key	 = p[1];
			auto const count = myClamp(p[2]);

			StopUnboundPredicate stop;

			accumulateResultsX<AccumulateOutput::BOTH_WITH_TAIL>(
				count					,
				stop					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				blob.container
			);

			return result.set_container(blob.container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xuget",	"XUGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XNGETKEYS : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 4)
				return;



			using namespace immutablex_impl_;

			static_assert(OutputBlob::ContainerSize >= 2 * ITERATIONS + 1);



			auto const key    = p[1];
			auto const count  = myClamp(p[2]);
			auto const prefix = p[3];

			if (prefix.empty())
				return;

			StopPrefixPredicate stop{ prefix };

			accumulateResultsXKeys<AccumulateOutput::KEYS_WITH_TAIL>(
				count					,
				stop					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				blob.container
			);

			return result.set_container(blob.container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xngetkeys",	"XNGETKEYS"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XRGETKEYS : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 4)
				return;



			using namespace immutablex_impl_;

			static_assert(OutputBlob::ContainerSize >= 2 * ITERATIONS + 1);



			auto const key   = p[1];
			auto const count = myClamp(p[2]);
			auto const end   = p[3];

			if (end.empty())
				return;

			StopRangePredicate stop{ end };

			accumulateResultsXKeys<AccumulateOutput::KEYS_WITH_TAIL>(
				count					,
				stop					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				blob.container
			);

			return result.set_container(blob.container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xrgetkeys",	"XRGETKEYS"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XUGETKEYS : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 3)
				return;



			using namespace immutablex_impl_;

			static_assert(OutputBlob::ContainerSize >= 2 * ITERATIONS + 1);



			auto const key	 = p[1];
			auto const count = myClamp(p[2]);

			StopUnboundPredicate stop;

			accumulateResultsXKeys<AccumulateOutput::KEYS_WITH_TAIL>(
				count					,
				stop					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				blob.container
			);

			return result.set_container(blob.container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xugetkeys",	"XUGETKEYS"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HGETALL : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 2)
				return;



			using namespace immutablex_impl_;

			static_assert(OutputBlob::ContainerSize >= 2 * ITERATIONS + 1);



			auto const keyN = p[1];

			if (keyN.empty())
				return;

			if (keyN.size() > MAX_HKEY_SIZE<DBAdapter>)
				return;

			auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR);

			accumulateResultsH<AccumulateOutput::BOTH>(
				ITERATIONS				,
				key					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				blob.container
			);

			return result.set_container(blob.container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hgetall",	"HGETALL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HGETKEYS : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 2)
				return;



			using namespace immutablex_impl_;

			static_assert(OutputBlob::ContainerSize >= 2 * ITERATIONS + 1);



			auto const keyN = p[1];

			if (keyN.empty())
				return;

			if (keyN.size() > MAX_HKEY_SIZE<DBAdapter>)
				return;

			auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR);

			accumulateResultsH<AccumulateOutput::KEYS>(
				ITERATIONS				,
				key					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				blob.container
			);

			return result.set_container(blob.container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hgetkeys",	"HGETKEYS"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HGETVALS : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 2)
				return;



			using namespace immutablex_impl_;

			static_assert(OutputBlob::ContainerSize >= 2 * ITERATIONS + 1);



			auto const keyN = p[1];

			if (keyN.empty())
				return;

			if (keyN.size() > MAX_HKEY_SIZE<DBAdapter>)
				return;

			auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR);

			accumulateResultsH<AccumulateOutput::VALS>(
				ITERATIONS				,
				key					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				blob.container
			);

			return result.set_container(blob.container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hgetvals",	"HGETVALS"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HLEN : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 2)
				return;



			using namespace immutablex_impl_;



			auto const keyN = p[1];

			if (keyN.empty())
				return;

			if (keyN.size() > MAX_HKEY_SIZE<DBAdapter>)
				return;

			auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR);

			auto const n = countResultsH(
				key					,
				db->find(key, std::false_type{})	,
				std::end(*db)
			);

			return result.set( uint64_t{ n } );
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hlen",	"HLEN"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "getx";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				XNGET		,
				XRGET		,
				XUGET		,

				XNGETKEYS	,
				XRGETKEYS	,
				XUGETKEYS	,

				HGETALL		,
				HGETKEYS	,
				HGETVALS	,
				HLEN
			>(pack);
		}
	};



} // namespace

