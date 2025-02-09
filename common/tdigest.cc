#include "tdigest.h"

#include <limits>
#include <cstdio>
#include <numeric>	// accumulate

#include "myendian_fp.h"

namespace {
	template<typename IT>
	void insertIntoSortedRange(IT first, IT last, typename std::iterator_traits<IT>::value_type &&item){
		auto it = std::lower_bound(first, last, item);
		std::move_backward(it, last, std::next(last));

		*it = std::move(item);
	}
} // namespace



struct Centroid{
	double		mean_;
	uint64_t	weight_;

	constexpr static auto create(double mean, uint64_t weight){
		return Centroid{
				htobe(mean),
				htobe(weight)
		};
	}

	constexpr void clear(){
		mean_   = 0;
		weight_ = 0;
	}

	constexpr auto mean() const{
		return betoh(mean_);
	}

	constexpr auto weight() const{
		return betoh(weight_);
	}

	constexpr operator bool() const{
		return weight_;
	}

	constexpr double weightedMean() const{
		return mean() * static_cast<double>(weight());
	}

	void print() const{
		printf("> Addr %p | mean: %10.4f | weight: %5zu\n", (void *) this, mean(), weight());
	}

	friend constexpr bool operator<(Centroid const &a, Centroid const &b){
		return a.mean() < b.mean();
	}

	friend constexpr bool operator<(Centroid const &a, double const mean){
		return a.mean() < mean;
	}
};



namespace {

	double findMinDistance(const Centroid *centroids, size_t const size){
		assert(size > 1);

		double minDistance = std::numeric_limits<double>::max();

		for(auto it = centroids; it != centroids + size - 1; ++it){
			auto const distance = std::abs(it->mean() - std::next(it)->mean());

			if (distance < minDistance)
				minDistance = distance;
		}

		return minDistance;
	}

	template<bool UseWeight>
	size_t compressCentroids_(Centroid *centroids, size_t size, double const delta){
		assert(size > 1);

		size_t newSize = 0;
		auto   current = centroids[0];

		auto const _ = [](double weight) -> double{
			if constexpr(UseWeight)
				return weight;
			else
				return 1.0;
		};

		for (size_t i = 1; i < size; ++i){
			auto const distance = std::abs(centroids[i].mean() - current.mean());
			auto const weight_u = current.weight() + centroids[i].weight();
			auto const weight   = static_cast<double>(weight_u);

			if (_(weight) * distance <= delta) {
				current = Centroid::create(
						(current.weightedMean() + centroids[i].weightedMean()) / weight,
						weight_u
				);
			}else{
				centroids[newSize++] = current;
				current = centroids[i];
			}
		}

		centroids[newSize++] = current;

		return newSize;
	}

	size_t compressNormal(Centroid *centroids, size_t size, double const delta){
		if (size < 2)
			return size;

		return compressCentroids_<1>(centroids, size, delta);
	}

	size_t compressAggressive(Centroid *centroids, size_t size, double const delta){
		if (size < 2)
			return size;

		auto const distance = findMinDistance(centroids, size);

		if (distance > delta)
			return compressCentroids_<0>(centroids, size, distance);
		else
			return compressCentroids_<1>(centroids, size, delta);
	}

} // namespace



struct RawTDigest::TDigest{
	uint64_t	size_;
	uint64_t	weight_;

	double		min_;
	double		max_;

	Centroid	data[1];

	constexpr auto size() const{
		return betoh(size_);
	}

	constexpr auto empty() const{
		return ! size_;
	}

public:
	constexpr auto weight() const{
		return betoh(weight_);
	}

	constexpr auto min() const{
		return betoh(min_);
	}

	constexpr auto max() const{
		return betoh(max_);
	}

public:
	constexpr auto setSize(uint64_t size){
		size_ = htobe(size);
	}

	constexpr auto setWeight(uint64_t weight){
		weight_ = htobe(weight);
	}

	void setMin_(double value){
		min_ = htobe(value);
	}

	void setMax_(double value){
		max_ = htobe(value);
	}
};



static_assert(std::is_trivial_v<RawTDigest::TDigest>);
static_assert(sizeof(RawTDigest::TDigest) == RawTDigest::bytes(1));



bool RawTDigest::valid(const TDigest *td) const{
	auto const size = td->size();

	return size <= capacity();
}

bool RawTDigest::empty(const TDigest *td) const{
	return ! td->size();
}

uint64_t RawTDigest::size(const TDigest *td) const{
	auto const size = td->size();

	return size <= capacity() ? size : 0;
}

uint64_t RawTDigest::weight(const TDigest *td) const{
	return valid(td) ? td->weight() : 0;
}

double RawTDigest::min(const TDigest *td) const{
	return valid(td) ? td->min() : 0;
}

double RawTDigest::max(const TDigest *td) const{
	return valid(td) ? td->max() : 0;
}

void RawTDigest::print(const TDigest *td) const{
	printf("Capacity %zu\n", capacity());

	for(size_t i = 0; i < size(td); ++i)
		td->data[i].print();
}



void RawTDigest::updateMinMax_(TDigest *td, double value) const{
	if (empty(td)){
		td->setMin_(value);
		td->setMax_(value);

		return;
	}

	if (value < td->min())
		td->setMin_(value);

	if (value > td->max())
		td->setMax_(value);
}



template<RawTDigest::Compression C>
void RawTDigest::add_(TDigest *td, double delta, double value, uint64_t weight) const{
	auto size = this->size(td);

	updateMinMax_(td, value);

	auto const oldWeight = empty(td) ? 0 : td->weight();

	auto insert = [&](){
		auto &centroids = td->data;

		insertIntoSortedRange(centroids, centroids + size, Centroid::create(value, weight));
		td->setSize	(size		+ 1		);
		td->setWeight	(oldWeight	+ weight	);
	};

	if (size < capacity())
		return insert();

	if constexpr(C == Compression::NONE)
		return;

	if constexpr(C == Compression::STANDARD)
		size = compressNormal		(td->data, size, delta);

	if constexpr(C == Compression::AGGRESSIVE)
		size = compressAggressive	(td->data, size, delta);

	if (size < capacity())
		return insert();

	// drop the value
	// should be unreachible, if Aggressive
	// no need to update size because size is unchanged
}



template<RawTDigest::Compression C>
void RawTDigest::add(TDigest *td, double delta, double value, uint64_t weight) const{
	assert(weight > 0);

	add_<C>(td, delta, value, weight);
}

template void RawTDigest::add<RawTDigest::Compression::NONE		>(TDigest *td, double delta, double value, uint64_t weight) const;
template void RawTDigest::add<RawTDigest::Compression::STANDARD		>(TDigest *td, double delta, double value, uint64_t weight) const;
template void RawTDigest::add<RawTDigest::Compression::AGGRESSIVE	>(TDigest *td, double delta, double value, uint64_t weight) const;



template<RawTDigest::Compression C>
void merge_(RawTDigest const &td_dest, RawTDigest::TDigest *dest, double delta, RawTDigest const &td_src, const RawTDigest::TDigest *src){
	if (! td_src.valid(src))
		return;

	auto &data = src->data;

	for(auto it = data; it < data + td_src.size(src); ++it)
		td_dest.add_<C>(dest, delta, it->mean(), it->weight());
}



template<RawTDigest::Compression C>
void merge(RawTDigest const &td_dest, RawTDigest::TDigest *dest, double delta, RawTDigest const &td_src, const RawTDigest::TDigest *src){
	return merge_<C>(td_dest, dest, delta, td_src, src);
}

template void merge<RawTDigest::Compression::NONE	>(RawTDigest const &td_dest, RawTDigest::TDigest *dest, double delta, RawTDigest const &td_src, const RawTDigest::TDigest *src);
template void merge<RawTDigest::Compression::STANDARD	>(RawTDigest const &td_dest, RawTDigest::TDigest *dest, double delta, RawTDigest const &td_src, const RawTDigest::TDigest *src);
template void merge<RawTDigest::Compression::AGGRESSIVE	>(RawTDigest const &td_dest, RawTDigest::TDigest *dest, double delta, RawTDigest const &td_src, const RawTDigest::TDigest *src);



void RawTDigest::compress(TDigest *td, double delta) const{
	auto const size = compressNormal(td->data, this->size(td), delta);
	td->setSize(size);
}



template<RawTDigest::Compression C>
void RawTDigest::merge(TDigest *dest, double delta, const TDigest *src) const{
	merge_<C>(*this, dest, delta, *this, src);
}

template void RawTDigest::merge<RawTDigest::Compression::NONE		>(TDigest *td, double delta, const TDigest *src) const;
template void RawTDigest::merge<RawTDigest::Compression::STANDARD	>(TDigest *td, double delta, const TDigest *src) const;
template void RawTDigest::merge<RawTDigest::Compression::AGGRESSIVE	>(TDigest *td, double delta, const TDigest *src) const;



double RawTDigest::percentile(const TDigest *td, double const p) const{
	if (empty(td))
		return 0;

	if (p == 0.0)
		return min(td);

	if (p == 1.0)
		return max(td);

	auto const size = this->size(td);

	double const targetRank = p * static_cast<double>(td->weight());
	double cumulativeWeight = 0;

	// size - 1, because we will return last element anyway.
	// array is not empty
	for(size_t i = 0; i < size - 1; ++i){
		cumulativeWeight += static_cast<double>(td->data[i].weight());
		if (cumulativeWeight >= targetRank)
			return td->data[i].mean();
	}

	return td->data[size - 1].mean();
}



double RawTDigest::trimmedMean_(const TDigest *td, double const minD, double const maxD) const{
	constexpr bool debug = false;

	if (empty(td))
		return 0;

	auto const w = static_cast<double>(weight(td));

	auto       min = w * minD;
	auto const max = w * maxD;

	auto const size = this->size(td);

	double cumulativeWeight	= 0;

	double sum		= 0.0;
	double count		= 0;

	for(size_t i = 0; i < size; ++i){
		cumulativeWeight += static_cast<double>(td->data[i].weight());

		if (cumulativeWeight < min){
			if constexpr(debug)
				printf("loop>>> %10zu | %10.4f | %10.4f | %10.4f |  %10.4f | %s\n", i, 0., cumulativeWeight, min, max, "skip");

			continue;
		}

		if (cumulativeWeight <= max){
			// normal delta, until end of the centroid
			auto const delta = cumulativeWeight - min;

			min = cumulativeWeight;

			sum   += td->data[i].mean() * delta;

			count += delta;

			if constexpr(debug)
				printf("loop>>> %10zu | %10.4f | %10.4f | %10.4f |  %10.4f | %s\n", i, delta, cumulativeWeight, min, max, "ongoing");

			continue;
		}else{
			// tricky delta, because is last centroid we will check
			auto const delta = max - min;

			sum += td->data[i].mean() * delta;

			count += delta;

			if constexpr(debug)
				printf("loop>>> %10zu | %10.4f | %10.4f | %10.4f |  %10.4f | %s\n", i, delta, cumulativeWeight, min, max, "final");

			break;
		}
	}

	if constexpr(debug)
		printf("finish> %10u | %10.4f | %10.4f | %10.4f | %10.4f | %10.4f\n", 0u, sum, count, sum / count, min, max);

	return sum / count;
}



double RawTDigest::mean(const TDigest *td) const{
	if (empty(td))
		return 0;

	auto const size = this->size(td);

	auto acc =  [](double a, Centroid const &c){
		return a + c.weightedMean();
	};

	auto const sum = std::accumulate(td->data, td->data + size, 0.0, acc);

	return sum / static_cast<double>(weight(td));
}


