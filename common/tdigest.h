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

	constexpr size_t capacity() const{
		return capacity_;
	}

	constexpr static size_t bytes(size_t capacity){
		return 2 * sizeof(uint64_t) + 2 * sizeof(double) + capacity * (sizeof(uint64_t) + sizeof(double));
	}

	constexpr size_t bytes() const{
		return bytes(capacity_);
	}

	void print(const TDigest *td) const;

	bool valid(const TDigest *td) const;

	bool empty(const TDigest *td) const;
	uint64_t size(const TDigest *td) const;
	uint64_t weight(const TDigest *td) const;
	double min(const TDigest *td) const;
	double max(const TDigest *td) const;

public:
	void clearFast(TDigest *td);

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

	void compress(TDigest *td, double delta) const;

	double percentile_50(const TDigest *td) const{
		return percentile(td, 0.50);
	}

	double percentile_95(const TDigest *td) const{
		return percentile(td, 0.95);
	}

	double percentile(const TDigest *td, double const p) const{
		assert(p >= 0.00 && p <= 1.00);

		return percentile_(td, p);
	}

	template<typename IT, typename OutIT>
	void percentile(const TDigest *td, IT first, IT last, OutIT out) const{
		auto f = [&](double p){
			assert(p >= 0.00 && p <= 1.00);
			return percentile_(td, p);
		};

		std::transform(first, last, out, f);
	}

private:
	double percentile_(const TDigest *td, double const p) const;
	void updateMinMax_(TDigest *td, double value) const;
	uint64_t weight_(const TDigest *td) const;

	template<Compression C>
	void add_(TDigest *td, double delta, double value, uint64_t weight) const;

};

#endif

