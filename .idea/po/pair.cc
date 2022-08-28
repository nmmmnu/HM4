#include "pair.h"
#include "pairblob.h"

#include "sgn.h"

#include <cassert>
#include <cstdio>

namespace hm4{

// ==============================

const Pair Pair::zero_ = {};

// ==============================

Pair::Pair() = default;

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

Pair::Pair(const Blob *blob, observer_tag) noexcept :
				pimpl(blob),
				observer_(true){}

// ==============================

Pair::Pair(const Pair &other) :
				Pair( other.pimpl.get() ){
	log__("copy c-tor\n");
}

Pair &Pair::operator=(const Pair &other){
	log__("copy assign\n");

	Pair pair = other;

	swap(pair);

	return *this;
}

Pair::Pair(Pair &&other) :
				pimpl		(std::move(other.pimpl		)),
				observer_	(std::move(other.observer_	)){
	log__("move c-tor\n");
}

Pair &Pair::operator=(Pair &&other){
	log__("move assign\n");

	swap(other);

	return *this;
}

// ==============================

void Pair::swap(Pair &other) noexcept{
	log__("swap\n");

	using std::swap;

	swap(pimpl,	other.pimpl	);
	swap(observer_,	other.observer_	);
}

// ==============================

Pair::~Pair(){
	if (observer_)
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

	return sgn(c1, c2);
}

bool Pair::isTombstone() const noexcept{
	return pimpl ?
		pimpl->isTombstone() :
		true;
}

bool Pair::isValid(bool const tombstoneCheck) const noexcept{
	return pimpl && pimpl->isValid(tombstoneCheck);
}

size_t Pair::bytes() const noexcept{
	return pimpl ?
		pimpl->bytes() :
		0;
}

size_t Pair::maxBytes() noexcept{
	return PairBlob::maxBytes();
}

void Pair::print() const noexcept{
	if (! pimpl){
		printf("--- %sPair is empty ---\n", observer_ ? "Observer " : "");
		return;
	}

	pimpl->print(observer_);
}

void Pair::fwrite(std::ostream & os) const{
	if (pimpl)
		os.write((const char *) pimpl.get(), (std::streamsize) bytes() );
}


} //namespace

