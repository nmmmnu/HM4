#ifndef MIN_MAX_HEAP_H_
#define MIN_MAX_HEAP_H_



#include <vector>
#include <utility>	// std::exchange
#include <stdexcept>



template<typename T>
struct MinMaxHeapCompare{
	constexpr static bool less(T const &a, T const &b){
		return a < b;
	}

	constexpr static bool greater(T const &a, T const &b){
		return a > b;
	}
};



// Based on:
// https://github.com/Bixkog/minmaxheap/blob/master/minmaxheap.h

template<typename T, typename Container = std::vector<T>, typename Compare = MinMaxHeapCompare<T> >
class MinMaxHeap{
public:
	using size_type = typename Container::size_type;

	MinMaxHeap() = default;

	MinMaxHeap(size_type const capacity){
		data_.reserve(capacity);
	}

	template<class UT>
	MinMaxHeap(Compare &&comp) : comp_(std::forward<UT>(comp)){
	}

	template<class UT>
	MinMaxHeap(UT &&comp, size_type const capacity) : MinMaxHeap(std::forward<UT>(comp)){
		data_.reserve(capacity);
	}

	template<typename UT>
	void insert(UT &&t);

	T const &getMax() const{
		// UB on empty heap

		return data_[0];
	}

	T const &getMin() const{
		// UB on empty heap

		if (size() == 1)
			return data_[0];
		else
			return data_[1];
	}

	T popMax();
	T popMin();

	size_type size() const{
		return data_.size();
	}

	bool empty() const{
		return data_.empty();
	}

private:
	void swap_(size_type a, size_type b){
		auto &_ = data_;

		using std::swap;
		swap(_[a], _[b]);
	}

	bool less_(size_type a, size_type b) const{
		if (a >= data_.size() || b >= data_.size()) {
			printf("BUG: less_ OOB! a=%zu, b=%zu, size=%zu\n",
				(size_t)a, (size_t)b, (size_t)data_.size());
		}


		auto &_ = data_;

		return comp_.less(_[a], _[b]);
	}

	bool greater_(size_type a, size_type b) const{
		if (a >= data_.size() || b >= data_.size()) {
			printf("BUG: greater_ OOB! a=%zu, b=%zu, size=%zu\n",
				(size_t)a, (size_t)b, (size_t)data_.size());
		}


		auto &_ = data_;

		return comp_.greater(_[a], _[b]);
	}

private:
	void moveUpH_(size_type k);
	void moveUpL_(size_type k);
	void moveDownH_(size_type k);
	void moveDownL_(size_type k);

	size_type partnerH_(size_type const k) const{
		auto const n = size();

		if(k == n - (n & 1)){
			size_type const base = k / 2;
			size_type result = base - 1 + (base & 1);

			if (result >= size()) {
				printf("BUG: partnerH_ out of bounds! k=%zu, n=%zu, base=%zu → result=%zu\n",
					(size_t)k, (size_t)n, (size_t)base, (size_t)result);
			}

			return result;
		}else{
			size_type result = k + 1;

			if (result >= size()) {
				printf("BUG: partnerH_ default out of bounds! k=%zu → result=%zu, size=%zu\n",
					(size_t)k, (size_t)result, (size_t)size());
			}

			return result;
		}
	}


	size_type partnerL_(size_type const k) const{
		auto const n = size();

		size_type const base = ((n - 1) / 2);

		if ((n & 1) == 1 && k == base - 1 + (base & 1))
			return n - 1;
		else
			return k - 1;
	}

	bool hasPartner_(size_type const k) const{
		auto const n = size();

		auto const c = getChildren__(k);

		if((k & 1) && (n & 1))
			return c.right >= n;
		else
			return c.left >= n;
	}

	constexpr static size_type parent__(size_type const k){
		size_type const base = k / 2;
		return base - 2 + (k & 1) + (base & 1);
	}

	struct Children{
		size_type left;
		size_type right;
	};

	constexpr static Children getChildren__(size_type const k){
		return {
			2 * (k + 1)     - (k & 1),
			2 * (k + 1) + 2 - (k & 1)
		};
	}

private:
	T popBack_(){
		T element = std::move(data_.back());
		data_.pop_back();
		return element;
	}

private:
	Container	data_;
	Compare		comp_;
};

// ===============================

template<typename T, typename Container, typename Compare>
template<typename UT>
void MinMaxHeap<T,Container,Compare>::insert(UT &&t){
	data_.push_back(std::forward<UT>(t));

	size_type const k = size() - 1;

	if(size() == 1)
		return;

	size_type const p = k & 1 ? partnerL_(k) : partnerH_(k);

	if(k & 1){
		if(greater_(k, p))
			moveDownL_(k);
		else
			moveUpL_(k);
	}else{
		if(less_(k, p))
			moveDownH_(k);
		else
			moveUpH_(k);
	}
}

template<typename T, typename Container, typename Compare>
T MinMaxHeap<T,Container,Compare>::popMax(){
	switch(size()){
	case 0:	throw std::logic_error("MinMaxHeap::popMax on empty heap");
	case 1: return popBack_();
	}

	T element = std::exchange(data_[0], std::move(data_.back()));
	data_.pop_back();
	moveDownH_(0);
	return element;
}

template<typename T, typename Container, typename Compare>
T MinMaxHeap<T,Container,Compare>::popMin(){
	switch(size()){
	case 0:	throw std::logic_error("MinMaxHeap::popMin on empty heap");
	case 1:
	case 2: return popBack_();
	}

	T element = std::exchange(data_[1], std::move(data_.back()));
	data_.pop_back();
	moveDownL_(1);
	return element;
}

// ===============================

template<typename T, typename Container, typename Compare>
void MinMaxHeap<T,Container,Compare>::moveUpH_(size_type k){
	size_type j;

	do{
		j = k;
		size_type const p = parent__(k);
		if(j > 1 && greater_(k, p)){
			k = p;

			swap_(k, j);
		}
	}while(j != k);
}

template<typename T, typename Container, typename Compare>
void MinMaxHeap<T,Container,Compare>::moveUpL_(size_type k){
	size_type j;

	do{
		j = k;
		size_type const p = parent__(k);
		if(j > 1 && less_(k, p)){
			k = p;

			swap_(k, j);
		}
	}while(j != k);
}

template<typename T, typename Container, typename Compare>
void MinMaxHeap<T,Container,Compare>::moveDownH_(size_type k){
	size_type j;

	do{
		j = k;

		auto const c = getChildren__(k);

		if(c.left  < size() && less_(k, c.left))
			k = c.left;

		if(c.right < size() && less_(k, c.right))
			k = c.right;

		swap_(k, j);
	}while(j != k);

	if(!hasPartner_(k))
		return;

	size_type const p = partnerH_(k);

	if(less_(k, p)){
		swap_(k, p);

		moveUpL_(p);
	}
}

template<typename T, typename Container, typename Compare>
void MinMaxHeap<T,Container,Compare>::moveDownL_(size_type k){
	size_type j;

	do{
		j = k;

		auto const c = getChildren__(k);

		if(c.left < size() && greater_(k, c.left))
			k = c.left;

		if(c.right < size()){
			if(greater_(k, c.right))
				k = c.right;

		}else if(c.left < size()){
			size_type const p = partnerL_(k);

			if(greater_(k, p)){
				swap_(j, p);
				moveUpH_(p);
				return;
			}
		}

		swap_(j, k);
	}while(j != k);

	if(!hasPartner_(k))
		return;

	size_type const p = partnerL_(k);

	if(greater_(k, p)){
		swap_(k, p);
		moveUpH_(p);
	}
}

#endif

