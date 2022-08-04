#include "pair.h"

#include "mytime.h"

#include <iostream>

namespace hm4{

inline namespace version_2_00_00{



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
	pair->expires	= htobe<uint32_t>(expires);
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
	const char *time_format = MyTime::TIME_FORMAT_STANDARD;
	const char *format      = "%-32s | %-20s | %s | %8u\n";
	const char *fnull	= "(null)";

	char buffer[MyTime::BUFFER_SIZE];

	printf(format,
		getKey_(),
		vallen ? getVal_() : fnull,
		MyTime::toString(buffer, getCreated(), time_format).data(),
		betoh<uint32_t>(expires)
	);
}

// ==============================

bool Pair::isExpired_() const noexcept{
	return expires &&  MyTime::expired( getCreated(), getExpires() );
}

uint64_t Pair::getCreateTime__(uint32_t const created) noexcept{
	return created ? MyTime::to64(created) : MyTime::now();
}

uint32_t Pair::getTTL() const noexcept{
	if (!expires)
		return 0;

	uint32_t const exp = getExpires();

	uint64_t const endTime64 = MyTime::addTime(getCreated(), exp);

	uint64_t const now64 = MyTime::now();

	return endTime64 < now64 ? 0 : MyTime::to32(endTime64 - now64);
}


} // anonymous namespace
} // namespace

