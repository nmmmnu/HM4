#ifndef _PAIR_H
#define _PAIR_H

#include "pairconf.h"

#include "stringref.h"

#include "mytime.h"

#include <ostream>
#include <memory>

// ==============================

namespace hm4{


struct PairBlob;


class Pair{
public:
	static constexpr uint16_t	MAX_KEY_SIZE	= PairConf::MAX_KEY_SIZE;
	static constexpr uint32_t	MAX_VAL_SIZE	= PairConf::MAX_VAL_SIZE;

private:
	static constexpr int CMP_ZERO = +1;

private:
	using Blob = PairBlob;

private:
	struct observer_pair{};

	Pair(const Blob *blob, const observer_pair&);

public:
	Pair(); /* = default */

	/* preconditons
	Key can not be zero length
	Key must be less MAX_KEY_SIZE
	Value must be less MAX_VAL_SIZE
	*/
	Pair(const StringRef &key, const StringRef &val, uint32_t expires = 0, uint32_t created = 0);

	Pair(const Blob *blob);

	static Pair observer(const Blob *blob){
		return Pair( blob, observer_pair{} );
	}

	static Pair tombstone(const StringRef &key){
		return Pair(key, StringRef{} );
	}

	Pair(const Pair &other);
	Pair &operator=(const Pair &other);

	Pair(Pair &&other); /* = default */
	Pair &operator=(Pair &&other); /* = default */

	~Pair(); /* = default */

	operator bool() const noexcept{
		return pimpl != nullptr;
	}

public:
	StringRef getKey() const noexcept;
	StringRef getVal() const noexcept;

	uint64_t getCreated() const noexcept;

	int cmp(const char *key, size_t size) const noexcept;
	int cmp(const char *key) const noexcept;

	int cmp(const StringRef &key) const noexcept{
		return cmp(key.data(), key.size());
	}

	int cmp(const Pair &pair) const noexcept{
		return cmp( pair.getKey() );
	}

	bool equals(const char *key, size_t size) const noexcept;
	bool equals(const char *key) const noexcept;

	bool equals(const StringRef &key) const noexcept{
		return equals(key.data(), key.size());
	}

	bool equals(const Pair &pair) const noexcept{
		return equals( pair.getKey() );
	}

	int cmpTime(const Pair &pair) const noexcept;

	bool isTombstone() const noexcept;

	bool valid(bool tombstoneCheck = false) const noexcept;

	bool valid(const Pair &, bool tombstoneCheck = false) const noexcept{
		return valid(tombstoneCheck);
	}

	size_t bytes() const noexcept;

public:
	bool fwrite(std::ostream & os) const;

	void print() const noexcept;

	void swap(Pair &other){
		std::swap(pimpl, other.pimpl);
	}

public:
	constexpr
	static const Pair &zero(){
		return zero_;
	}

private:
	std::unique_ptr<const Blob>	pimpl;
	bool				observer_ = false;

private:
	static const Pair	zero_;
};

// ==============================

inline void swap(Pair &p1, Pair &p2){
	p1.swap(p2);
}


} // namespace

#endif
