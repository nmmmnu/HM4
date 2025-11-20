#ifndef SHARED_ACCUMULATE_RESULTS_H_
#define SHARED_ACCUMULATE_RESULTS_H_

#include "shared_iterations.h"

namespace net::worker::shared::accumulate_results{

	using namespace net::worker::shared::config;
	using namespace net::worker::shared::stop_predicate;

	template<bool NoResultCount, class StopPredicate, class It, class ProcessPair, class ProcessTail>
	void sharedAccumulatePairs_(uint32_t const maxResults, StopPredicate stop, It it, It eit, ProcessTail pt, ProcessPair pp){
		uint32_t iterations	= 0;
		uint32_t results	= 0;

		for(;it != eit; ++it){
			auto const &key = it->getKey();

			if (++iterations > ITERATIONS_LOOPS_MAX)
				return pt(key);

			if (stop(key))
				return pt(); // no tail

			if (! it->isOK())
				continue;

			// use only pp accepts all data as results
			if constexpr(NoResultCount){
				if (++results > maxResults)
					return pt(key);
			}

			if (! pp(*it))
				return pt(key);
		}

		return pt();
	}

	template<class StopPredicate, class It, class ProcessPair, class ProcessTail>
	void sharedAccumulatePairs(                           StopPredicate stop, It it, It eit, ProcessTail pt, ProcessPair pp){
		uint32_t const maxResults = 0;
		return sharedAccumulatePairs_<0>(maxResults, stop, it, eit, pt, pp);
	}

	template<class StopPredicate, class It, class ProcessPair, class ProcessTail>
	void sharedAccumulatePairs(uint32_t const maxResults, StopPredicate stop, It it, It eit, ProcessTail pt, ProcessPair pp){
		return sharedAccumulatePairs_<1>(maxResults, stop, it, eit, pt, pp);
	}

	enum class AccumulateOutput{
		KEYS,
		KEYS_WITH_TAIL,
		VALS,
		BOTH,
		BOTH_WITH_TAIL
	};

	template<AccumulateOutput Out, class StopPredicate, class It, class Container, class ProjectionKey, class ProjectionKeyTail>
	void sharedAccumulateResults(uint32_t const maxResults, StopPredicate stop, It it, It eit, Container &container, ProjectionKey projKey, ProjectionKeyTail projKeyTail){

		auto pTail = [&](std::string_view const key = ""){
			if constexpr(is_any_of(Out, AccumulateOutput::BOTH_WITH_TAIL, AccumulateOutput::KEYS_WITH_TAIL)){
				auto const pkey = projKeyTail(key);
				container.emplace_back(key);
			}
		};

		auto pPair = [&](auto const &pair) mutable -> bool{
			if constexpr(is_any_of(Out, AccumulateOutput::BOTH_WITH_TAIL, AccumulateOutput::BOTH, AccumulateOutput::KEYS, AccumulateOutput::KEYS_WITH_TAIL)){
				auto const pkey = projKey(pair.getKey());
				container.emplace_back(pkey);
			}

			if constexpr(is_any_of(Out, AccumulateOutput::BOTH_WITH_TAIL, AccumulateOutput::BOTH, AccumulateOutput::VALS)){
				auto const &val = pair.getVal();
				container.emplace_back(val);
			}

			if constexpr(Out == AccumulateOutput::KEYS_WITH_TAIL){
				container.emplace_back("1");
			}

			return true;
		};

		sharedAccumulatePairs(maxResults, stop, it, eit, pTail, pPair);
	}

	template<AccumulateOutput Out, class StopPredicate, class It, class Container, class ProjectionKey>
	void sharedAccumulateResults(uint32_t const maxResults, StopPredicate stop, It it, It eit, Container &container, ProjectionKey projKey){
		auto projKeyTail = [](std::string_view const key){
			return key;
		};

		return sharedAccumulateResults<Out>(maxResults, stop, it, eit, container, projKey, projKeyTail);
	}

	template<AccumulateOutput Out, class StopPredicate, class It, class Container>
	void sharedAccumulateResults(uint32_t const maxResults, StopPredicate stop, It it, It eit, Container &container){
		auto projKey = [](std::string_view const key){
			return key;
		};

		return sharedAccumulateResults<Out>(maxResults, stop, it, eit, container, projKey);
	}

	template<class StopPredicate, class It>
	void sharedAccumulateResultsNext(std::string_view firstKey, StopPredicate stop, It it, It eit, std::array<std::string_view, 2> &container){
		uint32_t iterations	= 0;

		// capture & instead of &container to silence clang warning.
		auto tail = [&](std::string_view const pkey = ""){
			container = { "", pkey };
		};

		for(;it != eit; ++it){
			auto const &key = it->getKey();

			if (++iterations > ITERATIONS_RESULTS_MAX)
				return tail(key);

			if (stop(key))
				return tail(); // no tail

			if (! it->isOK())
				continue;

			if (key == firstKey) // skip first
				continue;

			auto const &val = it->getVal();

			container = { key, val };

			return;
		}

		return tail();
	}

} // namespace net::worker::shared::accumulate_results

#endif







#if 0
	template<AccumulateOutput Out, class StopPredicate, class It, class Container, class ProjectionKey, class ProjectionKeyTail>
	void sharedAccumulateResults____(uint32_t const maxResults, StopPredicate stop, It it, It eit, Container &container, ProjectionKey projKey, ProjectionKeyTail projKeyTail){
		uint32_t iterations	= 0;
		uint32_t results	= 0;

		// capture & instead of &container to silence clang warning.
		auto tail = [&](std::string_view const pkey = ""){
			if constexpr(is_any_of(Out, AccumulateOutput::BOTH_WITH_TAIL, AccumulateOutput::KEYS_WITH_TAIL))
				container.emplace_back(pkey);
		};

		for(;it != eit; ++it){
			auto const &key = it->getKey();

			auto const pkey = projKey(key);

			if (++iterations > ITERATIONS_RESULTS_MAX)
				return tail(projKeyTail(key));

			if (stop(key))
				return tail(); // no tail

			if (! it->isOK())
				continue;

			if (++results > maxResults)
				return tail(projKeyTail(key));

			auto const &val = it->getVal();

			if constexpr(is_any_of(Out, AccumulateOutput::BOTH_WITH_TAIL, AccumulateOutput::BOTH, AccumulateOutput::KEYS, AccumulateOutput::KEYS_WITH_TAIL))
				container.emplace_back(pkey);

			if constexpr(is_any_of(Out, AccumulateOutput::BOTH_WITH_TAIL, AccumulateOutput::BOTH, AccumulateOutput::VALS))
				container.emplace_back(val);

			if constexpr(Out == AccumulateOutput::KEYS_WITH_TAIL)
				container.emplace_back("1");
		}

		return tail();
	}
#endif


