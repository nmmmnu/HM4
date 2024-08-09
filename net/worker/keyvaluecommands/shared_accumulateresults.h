#ifndef SHARED_ACCUMULATE_RESULTS_H_
#define SHARED_ACCUMULATE_RESULTS_H_

#include "shared_iterations.h"

namespace net::worker::shared::accumulate_results{

	using namespace net::worker::shared::config;
	using namespace net::worker::shared::stop_predicate;

	enum class AccumulateOutput{
		KEYS,
		KEYS_WITH_TAIL,
		VALS,
		BOTH,
		BOTH_WITH_TAIL
	};

	template<AccumulateOutput Out, class StopPredicate, class It, class Container, class ProjectionKey>
	void accumulateResults(uint32_t const maxResults, StopPredicate stop, It it, It eit, Container &container, ProjectionKey projKey){
		uint32_t iterations	= 0;
		uint32_t results	= 0;

		// capture & instead of &container to silence clang warning.
		auto tail = [&](std::string_view const pkey = ""){
			if constexpr(is_any_of(Out, AccumulateOutput::BOTH_WITH_TAIL, AccumulateOutput::KEYS_WITH_TAIL))
				container.emplace_back(pkey);
		};

		for(;it != eit; ++it){
			auto const &key = it->getKey();

			auto pkey = projKey(key);

			if (++iterations > ITERATIONS_RESULTS_MAX)
				return tail(pkey);

			if (stop(key))
				return tail(); // no tail

			if (! it->isOK())
				continue;

			if (++results > maxResults)
				return tail(pkey);

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

	template<class StopPredicate, class It>
	void accumulateResultsNext(std::string_view firstKey, StopPredicate stop, It it, It eit, std::array<std::string_view, 2> &container){
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

