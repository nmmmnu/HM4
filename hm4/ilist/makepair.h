#ifndef SYSTEM_PAIR_H_
#define SYSTEM_PAIR_H_

#include "pair.h"

namespace hm4{

	Pair *makePairSystem(std::string_view name, PairBufferTombstone &buffer){
		auto *p = reinterpret_cast<Pair *>(buffer.data());

		Pair::createSystemPairInRawMemory(p, name);

		return p;
	}

	Pair *makePairTombstone(std::string_view name, PairBufferTombstone &buffer){
		auto *p = reinterpret_cast<Pair *>(buffer.data());

		Pair::createInRawMemoryFull(p, name, Pair::TOMBSTONE, 0, 0);

		return p;
	}

} // namespace hm4

#endif

