#include "pairblob.h"

#include "mytime.h"
#include "mynarrow.h"

#include <cstdio>

namespace hm4{


std::unique_ptr<PairBlob> PairBlob::create(
				const char *key, size_t const keylen,
				const char *val, size_t const vallen,
				uint32_t const expires, uint32_t const created){

	// preconditions
	if (	key == nullptr		||
		keylen == 0		||
		keylen > MAX_KEY_SIZE	||
		vallen > MAX_VAL_SIZE	)
		return {};
	// eo preconditions

	size_t const size = bytes(keylen, vallen);

	std::unique_ptr<PairBlob>  pair{ new(size) PairBlob };

	pair->created	= htobe64(getCreateTime__(created));
	pair->expires	= htobe32(expires);
	pair->vallen	= htobe32(narrow<uint32_t>(vallen));
	pair->keylen	= htobe16(narrow<uint16_t>(keylen));

	// memcpy so we can switch to blobs later...
	memcpy(& pair->buffer[0],		key, keylen);
	pair->buffer[keylen] = '\0';

	// this is safe with NULL pointer.
	memcpy(& pair->buffer[keylen + 1],	val, vallen);
	pair->buffer[keylen + 1 + vallen] = '\0';

	return pair;
}

std::unique_ptr<PairBlob> PairBlob::create(const PairBlob *src){
	if (src == nullptr)
		return {};

	size_t const size = src->bytes();

	std::unique_ptr<PairBlob> pair{ new(size) PairBlob };

	memcpy(pair.get(), src, size);

	return pair;
}

// ==============================

bool PairBlob::isExpired_() const noexcept{
	return expires &&  MyTime::expired( getCreated(), be32toh(expires) );
}

void PairBlob::print(bool const observer) const noexcept{
	const char *time_format = MyTime::TIME_FORMAT_STANDARD;
	const char *format      = "%-32s | %-20s | %s | %8u | %c\n";

	printf(format,
		getKey(), getVal(),
		MyTime::toString(getCreated(), time_format),
		be32toh(expires),
		observer ? '+' : ' '
	);
}

// ==============================

uint64_t PairBlob::getCreateTime__(uint32_t const created) noexcept{
	return created ? MyTime::combine(created) : MyTime::now();
}


} // namespace
