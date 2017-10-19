#ifndef _PAIR_NEW_H
#define _PAIR_NEW_H



#include <cstddef>	// offsetof
#include <memory>	// unique_ptr
#include <ostream>
#include "endian.h"

#include "stringref.h"

namespace hm4{
	namespace PairConf{
		constexpr uint16_t	ALIGN		= sizeof(uint64_t);
	}

struct Pair{
	uint64_t	created;	// 8
	uint32_t	expires;	// 4, 136 years, not that bad.
	uint32_t	vallen;		// 4
	uint16_t	keylen;		// 2
	char		buffer[1];	// dynamic

	// ==============================

public:
	static constexpr uint16_t	MAX_KEY_SIZE	=      1024;	// MySQL is 1000
	static constexpr uint32_t	MAX_VAL_SIZE	= 16 * 1024;

private:
	static constexpr int		CMP_NULLKEY	= -1;

public:
	static constexpr int		CMP_ZERO	= +1;

private:
	static const Pair		zero_;

private:
	Pair() noexcept = default;

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
	static std::unique_ptr<Pair> create(		const StringRef &key,
							const StringRef &val,
							uint32_t const expires = 0,
							uint32_t const created = 0){
		return create_(
			key.data(), key.size(),
			val.data(), val.size(),
			expires, created
		);
	}

	static std::unique_ptr<Pair> tombstone(const StringRef &key){
		return create(key, nullptr );
	}

	static std::unique_ptr<Pair> create(		const Pair *src);

	static std::unique_ptr<Pair> create(		const Pair &src){
		return create(&src);
	}

private:
	static std::unique_ptr<Pair> create_(	const char *key, size_t keylen,
							const char *val, size_t vallen,
							uint32_t expires,
							uint32_t created);

public:
	StringRef getKey() const noexcept{
		return { getKey_(), getKeyLen_() };
	}

	StringRef getVal() const noexcept{
		return { getVal_(), getValLen_() };
	}

	bool isTombstone() const noexcept{
		return vallen == 0;
	}

	uint64_t getCreated() const noexcept{
		return be64toh(created);
	}

public:
	int cmp(const char *key, size_t const size) const noexcept{
		return StringRef::fastEmptyChar(key, size) ?
			CMP_NULLKEY :
			StringRef::compare(getKey_(), getKeyLen_(), key, size);
	}

	int cmp(const char *key) const noexcept{
		return StringRef::fastEmptyChar(key) ?
			CMP_NULLKEY :
			StringRef::compare(getKey_(), getKeyLen_(), key, strlen(key) );
	}

	int cmp(const StringRef &key) const noexcept{
		return cmp(key.data(), key.size());
	}

	int cmp(const Pair &pair) const noexcept{
		return cmp( pair.getKey_(), getKeyLen_() );
	}

public:
	bool equals(const char *key, size_t const size) const noexcept{
		return StringRef::fastEmptyChar(key, size) ?
			false :
			StringRef::equals(getKey_(), getKeyLen_(), key, size);
	}

	bool equals(const char *key) const noexcept{
		return StringRef::fastEmptyChar(key) ?
			false :
			StringRef::equals(getKey_(), getKeyLen_(), key, strlen(key) );
	}

	bool equals(const StringRef &key) const noexcept{
		return equals(key.data(), key.size());
	}

	bool equals(const Pair &pair) const noexcept{
		return equals( pair.getKey_(), getKeyLen_() );
	}

public:
	int cmpTime(const Pair &pair) const noexcept;

public:
	operator bool() const noexcept{
		return keylen;
	}

	bool isValid(bool const tombstoneCheck = false) const noexcept{
		if ( tombstoneCheck && isTombstone() )
			return false;

		if ( isExpired_() )
			return false;

		// finally all OK
		return true;
	}

	bool isValid(const Pair &, bool const tombstoneCheck = false) const noexcept{
		return isValid(tombstoneCheck);
	}

	size_t bytes() const noexcept{
		return sizeofBase__() + sizeofBuffer_();
	}

	// ==============================

	void print() const noexcept;
	void fwrite(std::ostream & os) const;

	// ==============================

	constexpr
	static size_t maxBytes() noexcept{
		return sizeofBase__() + sizeofBuffer__(MAX_KEY_SIZE, MAX_VAL_SIZE);
	}

	// ==============================

	constexpr
	static const Pair &zero(){
		return zero_;
	}

private:
	const char *getKey_() const noexcept{
		return buffer;
	}

	const char *getVal_() const noexcept{
		// vallen is 0 no matter of endianness
		if (vallen == 0)
			return nullptr;

		return & buffer[ getKeyLen_() + 1 ];
	}

	size_t getKeyLen_() const noexcept{
		return be16toh(keylen);
	}

	size_t getValLen_() const noexcept{
		return be32toh(vallen);
	}

private:
	bool isExpired_() const noexcept;

	constexpr
	static size_t sizeofBase__() noexcept{
		return offsetof(Pair, buffer);
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
		return sizeofBuffer__(getKeyLen_(), getValLen_());
	}

	// ==============================

	static uint64_t getCreateTime__(uint32_t created) noexcept;

} __attribute__((__packed__));

static_assert(std::is_pod<Pair>::value, "Pair must be POD type");

} // namespace



#endif

