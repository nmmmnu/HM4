#ifndef _DUAL_MULTI_TABLE_ITERATOR_H
#define _DUAL_MULTI_TABLE_ITERATOR_H

#include "iteratorpair.h"

#include <variant>
#include <array>

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

public:
	using FirstIteratorPair		= typename multiiterator_impl_::IteratorPair<FirstIterator>;
	using SecondIteratorPair	= typename multiiterator_impl_::IteratorPair<SecondIterator>;

	using MyVariant			= std::variant<
						FirstIteratorPair,
						SecondIteratorPair
					>;
	using MyContainer		= std::array<MyVariant, 2>;

public:
	using difference_type	= it_traits_DT<FirstIterator>;
	using value_type	= const Pair;
	using pointer		= value_type *;
	using reference		= value_type &;
	using iterator_category	= std::forward_iterator_tag;

public:
	DualIterator(FirstIteratorPair first, SecondIteratorPair second){
		if (first)
			itp_[itpSize_++] = std::move(first);

		if (second)
			itp_[itpSize_++] = std::move(second);

		updateDereference_();
	}

	template<class List1, class List2, class... Args>
	DualIterator(List1 const &first, List2 const &second, Args&& ...args) :
					DualIterator(
						{ first,  std::forward<Args>(args)... },
						{ second, std::forward<Args>(args)... }
					){}

	DualIterator &operator++(){
		auto incPair = [](auto const &itp){
			const Pair *p;

			std::visit([&p](auto const &it){ ++it; p = *it; }, itp);

			return p;
		};

		if (itpSize_ == 0)
			return;

		MyContainer::size_type index = 0;

		const Pair *p = incPair(itp_[index]);

		if (!p


		// don't over optimize this...
		updateDereference_();

		return *this;
	}

	reference operator *() const{
		auto getPair = [](auto const &itp){
			const Pair *p;

			std::visit([&p](auto const &it){ p = *it; }, itp);

			return p;
		};

		return getPair(*itp_.front());
	}

	bool operator==(DualIterator const &other) const{
		return itpSize_ == 0 && other.itpSize_ == 0;
	}

public:
	bool operator!=(DualIterator const &other) const{
		return ! operator==(other);
	}

	pointer operator->() const{
		return & operator*();
	}

private:
	void updateDereference_(){
		if (itpSize_ < 2)
			return;

		auto getPair = [](auto const &itp){
			const Pair *p;

			std::visit([&p](auto const &it){ p = *it; }, itp);

			return p;
		};

		const Pair *p0 = getPair(itp_[0]);
		const Pair *p1 = getPair(itp_[1]);

		int const r = p0->cmp(*p1);

		if (r < 0){
			/* no swap */
			return;
		}

		if (r > 0){
			std::swap(itp_[0], itp_[1]);
			return;
		}

		int const rt = - p0->cmpTime(*p1);

		if (rt <= 0){
			/* no swap */
			return;
		}else{
			std::swap(itp_[0], itp_[1]);
			return;
		}
	}

private:
	MyContainer		itp_;
	MyContainer::size_type	itpSize_ = 0;

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

