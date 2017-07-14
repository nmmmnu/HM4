#include "pairblob.h"

#include "mytime.h"

#include <cstdio>

#include <cassert>

namespace hm4{

std::unique_ptr<PairBlob> PairBlob::create(
				const char *key, size_t const keylen,
				const char *val, size_t const vallen,
				uint32_t const expires, uint32_t const created){

	// preconditions

	assert(key != nullptr		);
	assert(keylen > 0		);
	assert(keylen < MAX_KEY_SIZE	);
	assert(vallen < MAX_VAL_SIZE	);

	// eo preconditions

	size_t const size = bytes(keylen, vallen);

	std::unique_ptr<PairBlob>  pair{ new(size) PairBlob };

	pair->created	= htobe64(getCreateTime__(created));
	pair->expires	= htobe32(expires);
	pair->vallen	= htobe32((uint32_t) vallen);
	pair->keylen	= htobe16((uint16_t) keylen);

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

bool PairBlob::valid(bool const tombstoneCheck) const noexcept{
	if ( tombstoneCheck && isTombstone() )
		return false;

	// now expires is 0 no matter of endianness
	if (expires && MyTime::expired( getCreated(), be32toh(expires) ) )
		return false;

	// finally all OK
	return true;
}

void PairBlob::print() const noexcept{
	static const char *format = "%-20s | %-20s | %-*s | %8u\n";

	printf(format,
		getKey(), getVal(),
		MyTime::STRING_SIZE, MyTime::toString(getCreated()),
		be32toh(expires)
	);
}

// ==============================

uint64_t PairBlob::getCreateTime__(uint32_t const created) noexcept{
	return created ? MyTime::combine(created) : MyTime::now();
}


} // namespace
