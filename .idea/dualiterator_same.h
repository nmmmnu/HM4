#include "fixedvector.h"
#include <vector>
#include <algorithm>	// heap

template <class Iterator>
class DualIterator<Iterator, Iterator>{
public:
	DualIterator(DualIterator const &other) = default;
	DualIterator(DualIterator &&other) = default;

private:
	template<class T>
	using it_traits_DT = typename std::iterator_traits<T>::difference_type;

public:
	using IteratorPair		= typename multiiterator_impl_::IteratorPair<Iterator>;

public:
	using difference_type	= it_traits_DT<Iterator>;
	using value_type	= const Pair;
	using pointer		= value_type *;
	using reference		= value_type &;
	using iterator_category	= std::forward_iterator_tag;

public:
	DualIterator(IteratorPair first, IteratorPair second) :
					tt_{
						std::move(first),
						std::move(second)
					}{
		sort_();
	}

	template<class List1, class List2, class... Args>
	DualIterator(List1 const &first, List2 const &second, Args&& ...args) :
					DualIterator(
						{ first,  std::forward<Args>(args)... },
						{ second, std::forward<Args>(args)... }
					){}

	DualIterator &operator++(){
		increment_();

		return *this;
	}

	reference operator *() const{
		return *tt_.front();
	}

	bool operator==(DualIterator const &other) const{
		return tt_ == other.tt_;
	}

public:
	bool operator!=(DualIterator const &other) const{
		return ! operator==(other);
	}

	pointer operator->() const{
		return & operator*();
	}

private:
	// heap compare
	static bool heap_comp(IteratorPair const &a, IteratorPair const &b){
		// returns ​true if the first argument is less than the second
		// however we negate this because we want min heap.
		return comp(a, b, std::true_type{}) > 0;
	}

	void sort_(){
		std::make_heap(std::begin(tt_), std::end(tt_), & DualIterator::heap_comp);
	}

	void increment_(){
		while(!tt_.empty()){
			std::pop_heap(std::begin(tt_), std::end(tt_), & DualIterator::heap_comp);

			auto &ip = tt_.back();

			// ip is guearanteed to be valid
			auto const &key = ip->getKey();

			++ip;

			if (ip){
				std::push_heap(std::begin(tt_), std::end(tt_), & DualIterator::heap_comp);
			}else{
				tt_.pop_back();

				if (tt_.empty())
					return;
			}

			if ( key != tt_.front()->getKey() ){
				// pair keys not match
				return;
			}
		}
	}

private:
//	FixedVector<IteratorPair, 2>	tt_;
	std::vector<IteratorPair>		tt_;
};
