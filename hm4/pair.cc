#include "pair.h"

#include "mytime.h"

#include <iostream>
#include <algorithm>	// std::min

namespace hm4{

inline namespace version_3_00_00{



void Pair::createInRawMemory(Pair *pair,
			std::string_view const key,
			std::string_view const val,
			uint32_t const expires, uint32_t const created) noexcept{

	// this is private and sizes are checked already

	uint16_t const keylen = uint16_t(
			(   key.size()					& PairConf::MAX_KEY_MASK	)	|
			( ( val.size() >> PairConf::MAX_VAL_MASK_SH )	& PairConf::MAX_VAL_MASK	)
	);

	uint16_t const vallen =
			val.size() & 0xffff
	;

	pair->created	= htobe<uint64_t>(getCreateTime__(created));
	pair->expires	= htobe<uint32_t>(std::min(expires, PairConf::EXPIRES_MAX));
	pair->keylen	= htobe<uint16_t>(keylen);
	pair->vallen	= htobe<uint16_t>(vallen);

	// memcpy so we can switch to blobs later...
	memcpy(& pair->buffer[0],		key.data(), key.size());
	pair->buffer[key.size()] = '\0';

	// this is safe with NULL pointer.
	memcpy(& pair->buffer[key.size() + 1],	val.data(), val.size());
	pair->buffer[key.size() + 1 + val.size()] = '\0';
}

// ==============================

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
	return expires &&  mytime::expired( getCreated(), getExpires() );
}

uint64_t Pair::getCreateTime__(uint32_t const created) noexcept{
	return created ? mytime::to64(created) : mytime::now();
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

