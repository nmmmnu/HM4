#ifndef _DUAL_MULTI_TABLE_ITERATOR_H
#define _DUAL_MULTI_TABLE_ITERATOR_H

#include "iteratorpair.h"

namespace hm4{
namespace multi{

template <class FirstIterator, class SecondIterator>
class DualIterator{
public:
	DualIterator(DualIterator const &other) = default;
	DualIterator(DualIterator &&other) = default;

private:
	template<class T>
	using it_traits_DT = typename std::iterator_traits<T>::difference_type;

	using FirstIteratorPair  = typename multiiterator_impl_::IteratorPair<FirstIterator>;
	using SecondIteratorPair = typename multiiterator_impl_::IteratorPair<SecondIterator>;

public:
	using difference_type	= it_traits_DT<FirstIterator>;
	using value_type	= const Pair;
	using pointer		= value_type *;
	using reference		= value_type &;
	using iterator_category	= std::forward_iterator_tag;

public:
	DualIterator(FirstIteratorPair first, SecondIteratorPair second) :
					first_(std::move(first)),
					second_(std::move(second)),
					pair_(dereference_()){}

	template<class List1, class List2, class... Args>
	DualIterator(List1 const &first, List2 const &second, Args&& ...args) :
					DualIterator(
						{ first,  std::forward<Args>(args)... },
						{ second, std::forward<Args>(args)... }
					){}

	DualIterator &operator++(){
		return increment_();
	}

	reference operator *() const{
		// normal iterator rules apply.
		return *pair_;
	}

	bool operator==(DualIterator const &other) const{
		return first_ == other.first_ && second_ == other.second_;
	}

public:
	bool operator!=(DualIterator const &other) const{
		return ! operator==(other);
	}

	pointer operator->() const{
		return & operator*();
	}

private:
	const Pair *dereference_() const{
		return multiiterator_impl_::comp(first_, second_) <= 0 ?
			first_.ptr() :
			second_.ptr()
		;
	}

	template<class T>
	DualIterator &incrementIterator_(T &it, const Pair *pair){
		++it;
		pair_ = pair;
		return *this;
	};

	template<class T>
	DualIterator &incrementIterator_(T &it){
		++it;
		pair_ = it.ptr();
		return *this;
	};

	DualIterator & increment_(){
		bool const f = first_;
		bool const s = second_;

		if (f == false && s == false)
			return *this;

		if (f && s == false)
			return incrementIterator_(first_);

		if (s && f == false)
			return incrementIterator_(second_);

		auto const cmp = first_->cmp(second_->getKey());

		if (cmp < 0)
			return incrementIterator_(first_, second_.ptr());

		if (cmp > 0)
			return incrementIterator_(second_, first_.ptr());

		// both pairs are equal, increase both

		++first_;
		++second_;

		// both iterators are new, could be false so...

		pair_ = dereference_();

		return *this;
	}

private:
	FirstIteratorPair	first_;
	SecondIteratorPair	second_;

	const Pair		*pair_;

private:
	static_assert(
		std::is_same<
			it_traits_DT<FirstIterator>,
			it_traits_DT<SecondIterator>
		>::value,
		"TABLE1::Iterator::difference_type must be same as TABLE2::iterator::difference_type"
	);
};

} // namespace multi
} // namespace

#endif

