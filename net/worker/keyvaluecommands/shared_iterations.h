#ifndef SHARED_ITERATIONS_H_
#define SHARED_ITERATIONS_H_

#include "base.h"

namespace net::worker::shared::config{
	using net::worker::commands::OutputBlob;

	constexpr uint32_t ITERATIONS_MIN	= 10;

	// This has nothing to do with the container size,
	// but to be unified with cmd_mutable.h
	constexpr uint32_t ITERATIONS_LOOPS	= OutputBlob::ContainerSize;

	// Results when pair-ed
	constexpr uint32_t ITERATIONS_RESULTS	= (OutputBlob::ContainerSize - 1) / 2;

	constexpr uint32_t ITERATIONS_IDLE	= 10;

} // namespace net::worker::shared::config


#endif

