#ifndef PAIR_POD_H
#define PAIR_POD_H

#include <cstddef>	// offsetof
#include <memory>	// unique_ptr
#include <ostream>
#include "myendian.h"
#include "my_void_t.h"
#include "mynarrow.h"
#include "comparator.h"

#include "stringref.h"

#define log__(...) /* nada */

namespace hm4{

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

public:
	static constexpr int		CMP_NULLKEY	= -1;

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
							uint32_t expires,
							uint32_t created){
		return 	create( key.data(), key.size(), val.data(), val.size(), expires, created );
	}

	static std::unique_ptr<Pair> create(		const char *key, size_t keylen,
							const char *val, size_t vallen,
							uint32_t expires,
							uint32_t created);

	static std::unique_ptr<Pair> create(const Pair *src);

	static std::unique_ptr<Pair> create(const Pair &src){
		return create(& src);
	}

public:
	constexpr
	bool empty() const noexcept{
		return keylen;
	}

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
	int cmp(const StringRef &key) const noexcept{
		return cmp_(key.data(), key.size());
	}

public:
	bool equals(const StringRef &key) const noexcept{
		return equals_(key.data(), key.size());
	}

public:
	int cmpTime(const Pair &pair) const noexcept{
		return comparator::comp(
			getCreated(),
			pair.getCreated()
		);
	}

	int cmpTime(const Pair *pair) const noexcept{
		return cmpTime(*pair);
	}

public:
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

	bool isValid(const Pair *pair, bool const tombstoneCheck = false) const noexcept{
		return isValid(*pair, tombstoneCheck);
	}

	size_t bytes() const noexcept{
		return sizeofBase__() + sizeofBuffer_();
	}

	// ==============================

	void print_() const noexcept;

	void fwrite(std::ostream & os) const{
		os.write((const char *) this, narrow<std::streamsize>( bytes() ) );
	}

	// ==============================

	constexpr
	static size_t maxBytes() noexcept{
		return sizeofBase__() + sizeofBuffer__(MAX_KEY_SIZE, MAX_VAL_SIZE);
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

	int cmp_(const char *key, size_t const size) const noexcept{
		return StringRef::fastEmptyChar(key, size) ?
			CMP_NULLKEY :
			StringRef::compare(getKey_(), getKeyLen_(), key, size);
	}

	bool equals_(const char *key, size_t const size) const noexcept{
		return StringRef::fastEmptyChar(key, size) ?
			false :
			StringRef::equals(getKey_(), getKeyLen_(), key, size);
	}

private:
	constexpr
	static size_t sizeofBase__() noexcept{
		return offsetof(Pair, buffer);
	}

	constexpr
	static size_t sizeofBuffer__(size_t const keyLen, size_t const valLen) noexcept{
		return keyLen + 1 + valLen + 1;
	}

	constexpr
	static size_t bytes_(size_t const keyLen, size_t const valLen) noexcept{
		return sizeofBase__() + sizeofBuffer__(keyLen, valLen);
	}

	// ==============================

	size_t sizeofBuffer_() const noexcept{
		return sizeofBuffer__(getKeyLen_(), getValLen_());
	}

	// ==============================

	bool isExpired_() const noexcept;

	static uint64_t getCreateTime__(uint32_t created) noexcept;

} __attribute__((__packed__));

static_assert(std::is_pod<Pair>::value, "Pair must be POD type");

// ==============================

inline void print(const Pair &pair){
	pair.print_();
}

template<class T>
void print(const T &ptr, std::nullptr_t){
	if (ptr)
		print(*ptr);
	else
		printf("--- pair is empty ---\n");
}

inline void print(const Pair *pair){
	print(pair, nullptr);
}

template<class T, class = my_void_t<decltype( *std::declval<T> )> >
void print(const T &it){
	print(*it);
}

} // namespace

#endif

