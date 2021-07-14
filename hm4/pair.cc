#include "pair.h"

#include "mytime.h"

namespace hm4{

Pair *Pair::createInRawMemory__(Pair *pair,
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

	return pair;
}

// ==============================

void Pair::print() const noexcept{
	const char *time_format = MyTime::TIME_FORMAT_STANDARD;
	const char *format      = "%-32s | %-20s | %s | %8u\n";
	const char *fnull	= "(null)";

	printf(format,
		getKey_(),
		vallen ? getVal_() : fnull,
		MyTime::toString(getCreated(), time_format),
		betoh<uint32_t>(expires)
	);
}

// ==============================

bool Pair::isExpired_() const noexcept{
	return expires &&  MyTime::expired( getCreated(), betoh<uint32_t>(expires) );
}

uint64_t Pair::getCreateTime__(uint32_t const created) noexcept{
	return created ? MyTime::combine(created) : MyTime::now();
}


} // namespace

