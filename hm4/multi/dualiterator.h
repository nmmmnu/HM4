#ifndef _DUAL_MULTI_TABLE_ITERATOR_H
#define _DUAL_MULTI_TABLE_ITERATOR_H

#include "iteratorpair.h"

namespace hm4{
namespace multi{



// it is tempting to make a specialization like this one
// template <class Iterator> class DualIterator<Iterator, Iterator>;
// but it does not worth at all.

template <class FirstIterator, class SecondIterator>
class DualIterator{
public:
	DualIterator(DualIterator const &other) = default;
	DualIterator(DualIterator &&other) = default;

private:
	template<class T>
	using it_traits_DT = typename std::iterator_traits<T>::difference_type;

public:
	using FirstIteratorPair		= typename multiiterator_impl_::IteratorPair<FirstIterator>;
	using SecondIteratorPair	= typename multiiterator_impl_::IteratorPair<SecondIterator>;

	using IteratorTuple		= std::pair<FirstIteratorPair, SecondIteratorPair>;
	using HitTuple			= std::array<bool, 2>;

public:
	using difference_type	= it_traits_DT<FirstIterator>;
	using value_type	= const Pair;
	using pointer		= value_type *;
	using reference		= value_type &;
	using iterator_category	= std::forward_iterator_tag;

public:
	DualIterator(FirstIteratorPair first, SecondIteratorPair second) :
					tt_(
						std::move(first),
						std::move(second)
					){
		updateDereference_();
	}

	template<class List1, class List2, class... Args>
	DualIterator(List1 const &first, List2 const &second, Args&& ...args) :
					DualIterator(
						{ first,  std::forward<Args>(args)... },
						{ second, std::forward<Args>(args)... }
					){}

	DualIterator &operator++(){
		if (hit_[0])
			++tt<0>();

		if (hit_[1])
			++tt<1>();;

		// don't over optimize this...
		updateDereference_();

		return *this;
	}

	reference operator *() const{
		return *pair_;
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
	template<size_t index>
	auto &tt(){
		return std::get<index>(tt_);
	}

//	template<size_t index>
//	auto &tt() const{
//		return std::get<index>(tt_);
//	}

	void updateDereference_(){
		constexpr bool X = true;
		constexpr bool _ = false;

		auto SP = [this](bool f, bool s, const Pair *p){
			hit_  = { f, s };
			pair_ = p;
		};

		auto S = [&SP](bool f, bool s, auto const &ip){
			SP(f, s, & *ip);
		};



		bool const bl[2] = { tt<0>(), tt<1>() };

		if (bl[0] && bl[1]){
			int const r = tt<0>()->cmp(*tt<1>());

			if (r < 0)
				return S(X,_, tt<0>());

			if (r > 0)
				return S(_,X, tt<1>());

			int const rt = - tt<0>()->cmpTime(*tt<1>());

			if (rt <= 0)
				return S(X,X, tt<0>());
			else
				return S(X,X, tt<1>());
		}

		if (bl[0])
			return S(X,_, tt<0>());

		if (bl[1])
			return S(_,X, tt<1>());

		return SP(_,_, nullptr);
	}

private:
	IteratorTuple	tt_;
	HitTuple	hit_;	// if we do not have this,

	const Pair      *pair_;

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

