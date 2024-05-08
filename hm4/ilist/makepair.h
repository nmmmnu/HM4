#ifndef SYSTEM_PAIR_H_
#define SYSTEM_PAIR_H_

#include "pair.h"

namespace hm4{

	Pair *makePairSystem(std::string_view name, char *buffer){
		auto *p = reinterpret_cast<Pair *>(buffer);

		Pair::createSystemPairInRawMemory(p, name);

		return p;
	}

	Pair *makePairSystem(std::string_view name, PairBufferTombstone &buffer){
		return makePairSystem(name, buffer.data());
	}

	Pair *makePairTombstone(std::string_view name, char *buffer){
		auto *p = reinterpret_cast<Pair *>(buffer);

		Pair::createInRawMemoryFull(p, name, Pair::TOMBSTONE, 0, 0);

		return p;
	}

	Pair *makePairTombstone(std::string_view name, PairBufferTombstone &buffer){
		return makePairTombstone(name, buffer.data());
	}

} // namespace hm4

#endif

