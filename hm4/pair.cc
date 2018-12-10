#include "pair.h"

#include "mytime.h"

namespace hm4{

std::unique_ptr<Pair> Pair::create(
				const char *key, size_t const keylen,
				const char *val, size_t const vallen,
				uint32_t const expires,
				uint32_t const created){

	if (	key == nullptr		||
		keylen == 0		||
		keylen > MAX_KEY_SIZE	||
		vallen > MAX_VAL_SIZE	)
		return {};

	size_t const size = bytes_(keylen, vallen);

	std::unique_ptr<Pair>  pair{ new(size) Pair };

	pair->created	= htobe<uint64_t>(getCreateTime__(created));
	pair->expires	= htobe<uint32_t>(expires);
	pair->vallen	= htobe<uint32_t>(narrow<uint32_t>(vallen));
	pair->keylen	= htobe<uint16_t>(narrow<uint16_t>(keylen));

	// memcpy so we can switch to blobs later...
	memcpy(& pair->buffer[0],		key, keylen);
	pair->buffer[keylen] = '\0';

	// this is safe with NULL pointer.
	memcpy(& pair->buffer[keylen + 1],	val, vallen);
	pair->buffer[keylen + 1 + vallen] = '\0';

	return pair;
}

std::unique_ptr<Pair> Pair::create(const Pair *src){
	if (src == nullptr)
		return {};

	size_t const size = src->bytes();

	std::unique_ptr<Pair> pair{ new(size) Pair };

	memcpy(pair.get(), src, size);

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



#if 0
	constexpr char Pair::ZERO[];

	constexpr static char ZERO[] = {
		0, 0, 0, 0,		// created
		0, 0, 0, 0,		// milliseconds
		0, 0, 0, 0,		// expires
		0, 0, 0, 0,		// vallen
		0, 0,			// keylen
		'\0',			// key
		'\0'			// val
	};

	constexpr static const Pair *zero_(){
		return reinterpret_cast<const Pair *>(ZERO);
	}
#endif

