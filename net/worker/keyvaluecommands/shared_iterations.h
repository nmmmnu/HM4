#ifndef SHARED_ITERATIONS_H_
#define SHARED_ITERATIONS_H_

#include "base.h"

namespace net::worker::shared::config{
	using net::worker::commands::OutputBlob;

	// Results when pair-ed
	constexpr uint32_t ITERATIONS_RESULTS_MIN	= 10;
	constexpr uint32_t ITERATIONS_RESULTS_MAX	= (OutputBlob::ContainerSize - 1) / 2;

	// This has nothing to do with the container size,
	// but to be unified with cmd_mutable.h
	constexpr uint32_t ITERATIONS_LOOPS_MIN		= ITERATIONS_RESULTS_MIN;
	constexpr uint32_t ITERATIONS_LOOPS_MAX		= OutputBlob::ContainerSize;

	constexpr uint32_t ITERATIONS_IDLE		= 10;

	constexpr uint32_t ITERATIONS_LOOPS_ACCUMULATE	= 1'000'000;

} // namespace net::worker::shared::config


#endif

