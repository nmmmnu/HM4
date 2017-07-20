#ifndef _PAIR_BLOB_H
#define _PAIR_BLOB_H

#include <cstddef>	// offsetof
#include <cstring>

#include "endian.h"

#include <memory>

#include "pairconf.h"

#include "stringref.h"

namespace hm4{

struct PairBlob{
	uint64_t	created;	// 8
	uint32_t	expires;	// 4, 136 years, not that bad.
	uint32_t	vallen;		// 4
	uint16_t	keylen;		// 2
	char		buffer[1];	// dynamic

	// ==============================

public:
	static constexpr uint16_t	MAX_KEY_SIZE	= PairConf::MAX_KEY_SIZE;
	static constexpr uint32_t	MAX_VAL_SIZE	= PairConf::MAX_VAL_SIZE;

private:
	static constexpr int		CMP_NULLKEY	= -1;

private:
	PairBlob() noexcept = default;

	static void *operator new(size_t, size_t const size){
		return ::operator new(size);
	}

	static void *operator new(size_t, size_t const size, const std::nothrow_t) noexcept{
		return ::operator new(size, std::nothrow);
	}

public:
	// fixing C++14 error
	static void operator delete(void* memory) noexcept{
		::operator delete(memory);
	}

public:
	static std::unique_ptr<PairBlob> create(	const char *key, size_t keylen,
							const char *val, size_t vallen,
							uint32_t expires, uint32_t created);

	static std::unique_ptr<PairBlob> create(	const PairBlob *src);

	static std::unique_ptr<PairBlob> create(	const std::unique_ptr<PairBlob> &src){
		return create(src.get());
	}

public:
	const char *getKey() const noexcept{
		return buffer;
	}

	const char *getVal() const noexcept{
		// vallen is 0 no matter of endianness
		if (vallen == 0)
			return nullptr;

		return & buffer[ getKeyLen() + 1 ];
	}

	size_t getKeyLen() const noexcept{
		return be16toh(keylen);
	}

	size_t getValLen() const noexcept{
		return be32toh(vallen);
	}

	bool isTombstone() const noexcept{
		return vallen == 0;
	}

	uint64_t getCreated() const noexcept{
		return be64toh(created);
	}

	int cmp(const char *key, size_t const size) const noexcept{
		return StringRef::fastEmptyChar(key, size) ?
			CMP_NULLKEY :
			StringRef::compare(getKey(), getKeyLen(), key, size);
	}

	int cmp(const char *key) const noexcept{
		return StringRef::fastEmptyChar(key) ?
			CMP_NULLKEY :
			StringRef::compare(getKey(), getKeyLen(), key, strlen(key) );
	}

	bool equals(const char *key, size_t const size) const noexcept{
		return StringRef::fastEmptyChar(key, size) ?
			bool(CMP_NULLKEY) :
			StringRef::equals(getKey(), getKeyLen(), key, size);
	}

	bool equals(const char *key) const noexcept{
		return StringRef::fastEmptyChar(key) ?
			bool(CMP_NULLKEY) :
			StringRef::equals(getKey(), getKeyLen(), key, strlen(key) );
	}

	bool valid(bool tombstoneCheck = false) const noexcept;

	size_t bytes() const noexcept{
		return sizeofBase__() + sizeofBuffer_();
	}

	// ==============================

	void print() const noexcept;

private:
	constexpr
	static size_t sizeofBase__() noexcept{
		return offsetof(PairBlob, buffer);
	}

	constexpr
	static size_t sizeofBuffer__(size_t const keyLen, size_t const valLen) noexcept{
		return keyLen + 1 + valLen + 1;
	}

	constexpr
	static size_t bytes(size_t const keyLen, size_t const valLen) noexcept{
		return sizeofBase__() + sizeofBuffer__(keyLen, valLen);
	}

	// ==============================

	size_t sizeofBuffer_() const noexcept{
		return sizeofBuffer__(getKeyLen(), getValLen());
	}

	// ==============================

	static uint64_t getCreateTime__(uint32_t created) noexcept;
} __attribute__((__packed__));

static_assert(std::is_pod<PairBlob>::value, "PairBlob must be POD type");

} // namespace

#endif

