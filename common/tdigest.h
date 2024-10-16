#ifndef T_DIGEST_H_
#define T_DIGEST_H_

#include <cstdint>
#include <cassert>
#include <cstring>
#include <algorithm>	// transform

class RawTDigest{
	size_t	capacity_;

public:
	struct TDigest;

	enum class Compression{
		NONE		,
		STANDARD	,
		AGGRESSIVE
	};

public:
	constexpr RawTDigest(size_t capacity) : capacity_(capacity){
		assert(capacity_ >= 2);
	}

	constexpr static size_t bytes(size_t capacity){
		size_t const base = 2 * sizeof(uint64_t) + 2 * sizeof(double);
		size_t const node = sizeof(uint64_t) + sizeof(double);

		return base + capacity * node;
	}

	constexpr static size_t maxCapacity(size_t memory){
		size_t const base = 2 * sizeof(uint64_t) + 2 * sizeof(double);
		size_t const node = sizeof(uint64_t) + sizeof(double);

		return (memory - base) / node;
	}

public:
	constexpr size_t capacity() const{
		return capacity_;
	}

	constexpr size_t bytes() const{
		return bytes(capacity_);
	}

	bool valid(const TDigest *td) const;

public:
	bool empty(const TDigest *td) const;
	uint64_t size(const TDigest *td) const;
	uint64_t weight(const TDigest *td) const;
	double min(const TDigest *td) const;
	double max(const TDigest *td) const;

	void print(const TDigest *td) const;

public:
	void clear(TDigest *td) const{
		memset(td, 0, bytes());
	}

	void load(TDigest *td, const void *src) const{
		memcpy(td, src, bytes());
	}

	void store(const TDigest *td, void *dest) const{
		memcpy(dest, td, bytes());
	}

public:
	template<Compression C = Compression::AGGRESSIVE>
	void add(TDigest *td, double delta, double value, uint64_t weight = 1) const;

	template<Compression C = Compression::AGGRESSIVE>
	void merge(TDigest *dest, double delta, const TDigest *src) const;

	template<Compression C>
	friend void merge_(RawTDigest const &td_dest, TDigest *dest, double delta, RawTDigest const &td_src, const TDigest *src);

	void compress(TDigest *td, double delta) const;

public:
	double percentile_50(const TDigest *td) const{
		return percentile(td, 0.50);
	}

	double percentile_95(const TDigest *td) const{
		return percentile(td, 0.95);
	}

	double percentile(const TDigest *td, double const p) const;

	double trimmedMean(const TDigest *td, double const min, double const max) const{
		assert(min >= 0.0 && min <= 1.0);
		assert(max >= 0.0 && max <= 1.0);

		if (min >= max)
			return 0;

		return trimmedMean_(td, min, max);
	}

	double mean(const TDigest *td) const;

private:
	void updateMinMax_(TDigest *td, double value) const;

	template<Compression C>
	void add_(TDigest *td, double delta, double value, uint64_t weight) const;

	double trimmedMean_(const TDigest *td, double const min, double const max) const;
};



template<RawTDigest::Compression C = RawTDigest::Compression::AGGRESSIVE>
void merge(RawTDigest const &td_dest, RawTDigest::TDigest *dest, double delta, RawTDigest const &td_src, const RawTDigest::TDigest *src);



#endif

