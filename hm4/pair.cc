#include "pair.h"
#include "pairblob.h"

#include <cassert>
#include <cstdio>		// printf for cygwin

namespace hm4{

// ==============================

const Pair Pair::zero_ = {};

// ==============================

Pair::Pair() = default;
Pair::Pair(Pair &&other) = default;
Pair &Pair::operator=(Pair &&other) = default;

// ==============================

Pair::Pair(const StringRef &key, const StringRef &val, uint32_t const expires, uint32_t const created):
	pimpl(
		Blob::create(	key.data(), key.size(),
				val.data(), val.size(),
				expires, created)
	){}

Pair::Pair(const Blob *blob) :
	pimpl(
		Blob::create(blob)
	){}

Pair::Pair(const Pair &other) :
	Pair(other.pimpl.get()){}

Pair &Pair::operator=(const Pair &other){
	Pair pair = Pair(other);

	swap(pair);

	return *this;
}

Pair::Pair(const Blob *blob, const observer_t&) :
				pimpl(blob),
				options_(Options::OBSERVER){}

#if 0
// very dangerous :)
Pair::Pair(const Pair &p, const observer_t&) :
				pimpl( p.pimpl.get() ),
				options_(Options::OBSERVER){}
#endif

// ==============================

Pair::~Pair(){
	if (options_ == Options::OBSERVER)
		pimpl.release();
}

// ==============================

StringRef Pair::getKey() const noexcept{
	return pimpl ?
		StringRef(pimpl->getKey(), pimpl->getKeyLen()) :
		StringRef();
}

StringRef Pair::getVal() const noexcept{
	return pimpl ?
		StringRef{ pimpl->getVal(), pimpl->getValLen() } :
		StringRef();
}

uint64_t Pair::getCreated() const noexcept{
	return pimpl ?
		pimpl->getCreated() :
		0;
}

int Pair::cmp(const char *key, size_t const size) const noexcept{
	return pimpl ?
		pimpl->cmp(key, size) :
		CMP_ZERO;
}

int Pair::cmp(const char *key) const noexcept{
	return pimpl ?
		pimpl->cmp(key) :
		CMP_ZERO;
}

bool Pair::equals(const char *key, size_t size) const noexcept{
	return pimpl ?
		pimpl->equals(key, size) :
		false;
}

bool Pair::equals(const char *key) const noexcept{
	return pimpl ?
		pimpl->equals(key) :
		false;
}

int Pair::cmpTime(const Pair &pair) const noexcept{
	// Compare time
	auto const c1 = getCreated();
	auto const c2 = pair.getCreated();

	if (c1 == c2)
		return 0;

	return c1 > c2 ? +1 : -1;
}

bool Pair::isTombstone() const noexcept{
	return pimpl ?
		pimpl->isTombstone() :
		true;
}

bool Pair::valid(bool const tombstoneCheck) const noexcept{
	return pimpl ?
		pimpl->valid(tombstoneCheck) :
		false;
}

size_t Pair::bytes() const noexcept{
	return pimpl ?
		pimpl->bytes() :
		0;
}

void Pair::print() const noexcept{
	if (pimpl == nullptr){
		printf("--- Pair is empty ---\n");
		return;
	}

	pimpl->print();
}

bool Pair::fwrite(std::ostream & os) const{
	if (pimpl == nullptr)
		return false;

	os.write((const char *) pimpl.get(), (std::streamsize) bytes() );

	return true;
}

} //namespace

