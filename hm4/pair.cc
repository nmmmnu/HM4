#include "pair.h"

#include "mytime.h"

namespace hm4{

Pair *Pair::copy_(Pair *pair,
			std::string_view const key,
			std::string_view const val,
			uint32_t const expires, uint32_t const created){

	pair->created	= htobe<uint64_t>(getCreateTime__(created));
	pair->expires	= htobe<uint32_t>(expires);
	pair->vallen	= htobe<uint32_t>(narrow<uint32_t>(val.size()));
	pair->keylen	= htobe<uint16_t>(narrow<uint16_t>(key.size()));

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

	printf(format,
		getKey_(), getVal_(),
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

