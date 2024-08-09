#include "base.h"
#include "mystring.h"
#include "logger.h"
#include "my_type_traits.h"

#include <algorithm>	// std::clamp

#include "shared_stoppredicate.h"
#include "shared_accumulateresults.h"

namespace net::worker::commands::ImmutableX{
	namespace immutablex_impl_{

		using namespace net::worker::shared::accumulate_results;
		using namespace net::worker::shared::config;



		template<AccumulateOutput Out, class StopPredicate, class It, class Container>
		void accumulateResultsX(uint32_t const maxResults, StopPredicate stop, It it, It eit, Container &container){
			auto proj = [](std::string_view x){
				return x;
			};

			return accumulateResults<Out>(maxResults, stop, it, eit, container, proj);
		}

		template<AccumulateOutput Out, class It, class Container>
		void accumulateResultsH(uint32_t const maxResults, std::string_view const prefix, It it, It eit, Container &container){

			auto proj = [prefix](std::string_view x) -> std::string_view{
				return after_prefix(prefix, x);
			};

			StopPrefixPredicate stop{ prefix };

			return accumulateResults<Out>(maxResults, stop, it, eit, container, proj);
		}



		template<class It>
		uint32_t countResultsH(std::string_view const prefix, It it, It eit){
			uint32_t iterations	= 0;
			uint32_t results	= 0;

			StopPrefixPredicate stop{ prefix };

			for(;it != eit;++it){
				auto const &key = it->getKey();

				// should be ITERATIONS_LOOPS,
				// but we use ITERATIONS_RESULTS to be same as if accumulated
				if (++iterations > ITERATIONS_RESULTS_MAX)
					break;

				if (stop(key))
					break;

				if (! it->isOK())
					continue;

				++results;
			}

			return results;
		}

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
			using namespace immutablex_impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);



			auto const key    = p[1];
			auto const count  = myClamp<uint32_t>(p[2], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const prefix = p[3];

			if (prefix.empty())
				return result.set_error(ResultErrorMessages::EMPTY_PREFIX);

			StopPrefixPredicate stop{ prefix };

			auto &container = blob.container();

			accumulateResultsX<AccumulateOutput::BOTH_WITH_TAIL>(
				count					,
				stop					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				container
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xnget",	"XNGET",
			"getx",		"GETX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XNNEXT : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace immutablex_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);



			auto const key    = p[1];
			auto const prefix = p[2];

			if (prefix.empty())
				return result.set_error(ResultErrorMessages::EMPTY_PREFIX);

			StopPrefixPredicate stop{ prefix };

			std::array<std::string_view, 2> container;

			accumulateResultsNext(
				key					,
				stop					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				container
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xnnext",	"XNNEXT"
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
			using namespace immutablex_impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);



			auto const key		= p[1];
			auto const count	= myClamp<uint32_t>(p[2], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const keyEnd	= p[3];

			if (!hm4::Pair::isKeyValid(keyEnd))
				return result.set_error(ResultErrorMessages::EMPTY_ENDCOND);

			StopRangePredicate stop{ keyEnd };

			auto &container = blob.container();

			accumulateResultsX<AccumulateOutput::BOTH_WITH_TAIL>(
				count					,
				stop					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				container
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xrget",	"XRGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XRNEXT : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace immutablex_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);



			auto const key    = p[1];
			auto const keyEnd = p[2];

			if (!hm4::Pair::isKeyValid(keyEnd))
				return result.set_error(ResultErrorMessages::EMPTY_ENDCOND);

			StopRangePredicate stop{ keyEnd };

			std::array<std::string_view, 2> container;

			accumulateResultsNext(
				key					,
				stop					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				container
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xrnext",	"XRNEXT"
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
			using namespace immutablex_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);



			auto const key	 = p[1];
			auto const count = myClamp<uint32_t>(p[2], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);

			StopUnboundPredicate stop;

			auto &container = blob.container();

			accumulateResultsX<AccumulateOutput::BOTH_WITH_TAIL>(
				count					,
				stop					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				container
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xuget",	"XUGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XUNEXT : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace immutablex_impl_;

			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);



			auto const key    = p[1];

			StopUnboundPredicate stop;

			std::array<std::string_view, 2> container;

			accumulateResultsNext(
				key					,
				stop					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				container
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xunext",	"XUNEXT"
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
			using namespace immutablex_impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);



			auto const key    = p[1];
			auto const count  = myClamp<uint32_t>(p[2], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const prefix = p[3];

			if (prefix.empty())
				return result.set_error(ResultErrorMessages::EMPTY_PREFIX);

			StopPrefixPredicate stop{ prefix };

			auto &container = blob.container();

			accumulateResultsX<AccumulateOutput::KEYS_WITH_TAIL>(
				count					,
				stop					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				container
			);

			return result.set_container(container);
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
			using namespace immutablex_impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);



			auto const key		= p[1];
			auto const count	= myClamp<uint32_t>(p[2], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const keyEnd	= p[3];

			if (!hm4::Pair::isKeyValid(keyEnd))
				return result.set_error(ResultErrorMessages::EMPTY_ENDCOND);

			StopRangePredicate stop{ keyEnd };

			auto &container = blob.container();

			accumulateResultsX<AccumulateOutput::KEYS_WITH_TAIL>(
				count					,
				stop					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				container
			);

			return result.set_container(container);
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
			using namespace immutablex_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);



			auto const key	 = p[1];
			auto const count = myClamp<uint32_t>(p[2], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);

			StopUnboundPredicate stop;

			auto &container = blob.container();

			accumulateResultsX<AccumulateOutput::KEYS_WITH_TAIL>(
				count					,
				stop					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				container
			);

			return result.set_container(container);
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
			using namespace immutablex_impl_;

			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);



			auto const keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!hm4::Pair::isKeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			hm4::PairBufferKey bufferKey;
			auto const key = concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR);

			auto &container = blob.container();

			accumulateResultsH<AccumulateOutput::BOTH>(
				ITERATIONS_RESULTS_MAX			,
				key					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				container
			);

			return result.set_container(container);
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
			using namespace immutablex_impl_;

			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);



			auto const keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!hm4::Pair::isKeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			hm4::PairBufferKey bufferKey;
			auto const key = concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR);

			auto &container = blob.container();

			accumulateResultsH<AccumulateOutput::KEYS>(
				ITERATIONS_RESULTS_MAX			,
				key					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				container
			);

			return result.set_container(container);
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
			using namespace immutablex_impl_;

			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);



			auto const keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!hm4::Pair::isKeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			hm4::PairBufferKey bufferKey;
			auto const key = concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR);

			auto &container = blob.container();

			accumulateResultsH<AccumulateOutput::VALS>(
				ITERATIONS_RESULTS_MAX			,
				key					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				container
			);

			return result.set_container(container);
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

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);



			using namespace immutablex_impl_;



			auto const keyN = p[1];

			if (!hm4::Pair::Pair::isKeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			hm4::PairBufferKey bufferKey;
			auto const key = concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR);

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
		constexpr inline static std::string_view name	= "immutable_x";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				XNGET		,
				XRGET		,
				XUGET		,

				XNGETKEYS	,
				XRGETKEYS	,
				XUGETKEYS	,

				XNNEXT		,
				XRNEXT		,
				XUNEXT		,

				HGETALL		,
				HGETKEYS	,
				HGETVALS	,
				HLEN
			>(pack);
		}
	};



} // namespace

