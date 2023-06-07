#include "pair.h"

#include "mytime.h"

#include <iostream>
#include <algorithm>	// std::min

namespace hm4{

inline namespace version_4_00_00{



void Pair::print() const noexcept{
	const char *format      = "%-32s | %-20s | %s | %8u\n";
	const char *fnull	= "(null)";

	mytime::to_string_buffer_t buffer;

	printf(format,
		getKey_(),
		vallen ? getVal_() : fnull,
		mytime::toString(getCreated(), mytime::TIME_FORMAT_STANDARD, buffer).data(),
		betoh<uint32_t>(expires)
	);
}

// ==============================

bool Pair::isExpired_() const noexcept{
	// beware problem 2038 !!!
	return expires && mytime::expired( getCreated(), getExpires() );
}

uint64_t Pair::prepareCreateTime(uint32_t const created) noexcept{
	return created ? mytime::to64(created) : mytime::now();
}

uint64_t Pair::prepareCreateTimeSimulate(uint32_t const created) noexcept{
	return created ? mytime::to64(created) : std::numeric_limits<uint64_t>::max();
}

uint32_t Pair::getTTL() const noexcept{
	if (!expires)
		return 0;

	uint32_t const exp = getExpires();

	uint64_t const endTime64 = mytime::addTime(getCreated(), exp);

	uint64_t const now64 = mytime::now();

	return endTime64 < now64 ? 0 : mytime::to32(endTime64 - now64);
}


} // anonymous namespace
} // namespace

