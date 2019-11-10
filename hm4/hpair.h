#ifndef CPAIR_H
#define CPAIR_H

#include "pair.h"

#include "stringhash.h"

namespace hm4{

	namespace HPair{
		using HKey	= uint64_t;
		using SS	= StringHash<HKey>;
	} // namespace

} // namespace hm4

#endif

