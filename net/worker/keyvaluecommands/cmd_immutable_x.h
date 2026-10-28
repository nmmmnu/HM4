#include "base.h"
#include "mystring.h"
#include "logger.h"
#include "my_type_traits.h"

#include <algorithm>	// std::clamp

#include "shared_stoppredicate.h"
#include "shared_accumulateresults.h"

namespace net::worker::commands::ImmutableX{
	namespace impl_{

		using namespace net::worker::shared::accumulate_results;
		using namespace net::worker::shared::config;



		template<AccumulateOutput Out, class StopPredicate, class It, class Container>
		void accumulateResultsX(uint32_t const maxResults, StopPredicate stop, It it, It eit, Container &container){
			return sharedAccumulateResults<Out>(maxResults, stop, it, eit, container);
		}

		template<AccumulateOutput Out, class It, class Container>
		void accumulateResultsH(uint32_t const maxResults, std::string_view const prefix, It it, It eit, Container &container){

			auto proj = [prefix](std::string_view x) -> std::string_view{
				return after_prefix(prefix, x);
			};

			StopPrefixPredicate stop{ prefix };

			return sharedAccumulateResults<Out>(maxResults, stop, it, eit, container, proj);
		}



		template<class It>
		uint32_t countResultsH(std::string_view const prefix, It it, It eit){
			uint32_t iterations	= 0;
			uint32_t results	= 0;

			StopPrefixPredicate stop{ prefix };

			for(;it != eit; ++it){
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

	} // namespace impl_



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XNGET : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XNGET() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XNGET", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);



			auto const key    = p[1];
			auto const count  = myClamp<uint32_t>(p[2], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const prefix = p[3];

			if (prefix.empty())
				return result.set_error(ResultErrorMessages::EMPTY_PREFIX);

			StopPrefixPredicate stop{ prefix };

			auto &container = blob.construct<OutputBlob::Container>();

			accumulateResultsX<AccumulateOutput::BOTH_WITH_TAIL>(
				count		,
				stop		,
				db->find(key)	,
				std::end(*db)	,
				container
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xnget",	"XNGET",
			"getx",		"GETX"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XNNEXT : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XNNEXT() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XNNEXT", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){
			using namespace impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);



			auto const key    = p[1];
			auto const prefix = p[2];

			if (prefix.empty())
				return result.set_error(ResultErrorMessages::EMPTY_PREFIX);

			StopPrefixPredicate stop{ prefix };

			std::array<std::string_view, 2> container;

			sharedAccumulateResultsNext(
				key		,
				stop		,
				db->find(key)	,
				std::end(*db)	,
				container
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xnnext",	"XNNEXT"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XRGET : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XRGET() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XRGET", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);



			auto const key		= p[1];
			auto const count	= myClamp<uint32_t>(p[2], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const keyEnd	= p[3];

			if (!hm4::Pair::isKeyValid(keyEnd))
				return result.set_error(ResultErrorMessages::EMPTY_ENDCOND);

			StopRangePredicate stop{ keyEnd };

			auto &container = blob.construct<OutputBlob::Container>();

			accumulateResultsX<AccumulateOutput::BOTH_WITH_TAIL>(
				count		,
				stop		,
				db->find(key)	,
				std::end(*db)	,
				container
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xrget",	"XRGET"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XRNEXT : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XRNEXT() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XRNEXT", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){
			using namespace impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);



			auto const key    = p[1];
			auto const keyEnd = p[2];

			if (!hm4::Pair::isKeyValid(keyEnd))
				return result.set_error(ResultErrorMessages::EMPTY_ENDCOND);

			StopRangePredicate stop{ keyEnd };

			std::array<std::string_view, 2> container;

			sharedAccumulateResultsNext(
				key		,
				stop		,
				db->find(key)	,
				std::end(*db)	,
				container
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xrnext",	"XRNEXT"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XUGET : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XUGET() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XUGET", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);



			auto const key	 = p[1];
			auto const count = myClamp<uint32_t>(p[2], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);

			StopUnboundPredicate stop;

			auto &container = blob.construct<OutputBlob::Container>();

			accumulateResultsX<AccumulateOutput::BOTH_WITH_TAIL>(
				count		,
				stop		,
				db->find(key)	,
				std::end(*db)	,
				container
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xuget",	"XUGET"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XUNEXT : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XUNEXT() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XUNEXT", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){
			using namespace impl_;

			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);



			auto const key    = p[1];

			StopUnboundPredicate stop;

			std::array<std::string_view, 2> container;

			sharedAccumulateResultsNext(
				key		,
				stop		,
				db->find(key)	,
				std::end(*db)	,
				container
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xunext",	"XUNEXT"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XNGETKEYS : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XNGETKEYS() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XNGETKEYS", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);



			auto const key    = p[1];
			auto const count  = myClamp<uint32_t>(p[2], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const prefix = p[3];

			if (prefix.empty())
				return result.set_error(ResultErrorMessages::EMPTY_PREFIX);

			StopPrefixPredicate stop{ prefix };

			auto &container = blob.construct<OutputBlob::Container>();

			accumulateResultsX<AccumulateOutput::KEYS_WITH_TAIL>(
				count		,
				stop		,
				db->find(key)	,
				std::end(*db)	,
				container
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xngetkeys",	"XNGETKEYS"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XRGETKEYS : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XRGETKEYS() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XRGETKEYS", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);



			auto const key		= p[1];
			auto const count	= myClamp<uint32_t>(p[2], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const keyEnd	= p[3];

			if (!hm4::Pair::isKeyValid(keyEnd))
				return result.set_error(ResultErrorMessages::EMPTY_ENDCOND);

			StopRangePredicate stop{ keyEnd };

			auto &container = blob.construct<OutputBlob::Container>();

			accumulateResultsX<AccumulateOutput::KEYS_WITH_TAIL>(
				count		,
				stop		,
				db->find(key)	,
				std::end(*db)	,
				container
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xrgetkeys",	"XRGETKEYS"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XUGETKEYS : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XUGETKEYS() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XUGETKEYS", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);



			auto const key	 = p[1];
			auto const count = myClamp<uint32_t>(p[2], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);

			StopUnboundPredicate stop;

			auto &container = blob.construct<OutputBlob::Container>();

			accumulateResultsX<AccumulateOutput::KEYS_WITH_TAIL>(
				count		,
				stop		,
				db->find(key)	,
				std::end(*db)	,
				container
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xugetkeys",	"XUGETKEYS"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct HGETALL : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		HGETALL() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("HGETALL", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);



			auto const keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!hm4::Pair::isKeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			hm4::PairBufferKey bufferKey;
			auto const key = concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR);

			auto &container = blob.construct<OutputBlob::Container>();

			accumulateResultsH<AccumulateOutput::BOTH>(
				ITERATIONS_RESULTS_MAX	,
				key			,
				db->find(key)		,
				std::end(*db)		,
				container
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"hgetall",	"HGETALL"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct HGETKEYS : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		HGETKEYS() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("HGETKEYS", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);



			auto const keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!hm4::Pair::isKeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			hm4::PairBufferKey bufferKey;
			auto const key = concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR);

			auto &container = blob.construct<OutputBlob::Container>();

			accumulateResultsH<AccumulateOutput::KEYS>(
				ITERATIONS_RESULTS_MAX	,
				key			,
				db->find(key)		,
				std::end(*db)		,
				container
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"hgetkeys",	"HGETKEYS"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct HGETVALS : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		HGETVALS() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("HGETVALS", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);



			auto const keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!hm4::Pair::isKeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			hm4::PairBufferKey bufferKey;
			auto const key = concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR);

			auto &container = blob.construct<OutputBlob::Container>();

			accumulateResultsH<AccumulateOutput::VALS>(
				ITERATIONS_RESULTS_MAX	,
				key			,
				db->find(key)		,
				std::end(*db)		,
				container
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"hgetvals",	"HGETVALS"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct HLEN : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		HLEN() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("HLEN", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);



			using namespace impl_;



			auto const keyN = p[1];

			if (!hm4::Pair::Pair::isKeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			hm4::PairBufferKey bufferKey;
			auto const key = concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR);

			auto const n = countResultsH(
				key		,
				db->find(key)	,
				std::end(*db)
			);

			return result.set( uint64_t{ n } );
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"hlen",	"HLEN"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter, typename RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "immutable_x";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, MemoryManager, DBAdapter, RegisterPack,
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

