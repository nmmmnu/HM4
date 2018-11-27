#ifndef _DUAL_MULTI_TABLE_ITERATOR_H
#define _DUAL_MULTI_TABLE_ITERATOR_H

#include "basemultiiterator_.h"

namespace hm4{
namespace multi{

template <class TABLE1, class TABLE2>
class DualIterator{
public:
	DualIterator(DualIterator const &other) = default;
	DualIterator(DualIterator &&other) = default;

private:
	template<class T>
	using it_traits = std::iterator_traits<typename T::Iterator>;

	static_assert(
		std::is_same<
			typename it_traits<TABLE1>::difference_type,
			typename it_traits<TABLE2>::difference_type
		>::value,
	"TABLE1::Iterator::difference_type must be same as TABLE2::Iterator::difference_type");

	template<class T>
	using is_input_iterator =
		std::is_base_of<
			std::input_iterator_tag,
			typename it_traits<T>::iterator_category
		>
	;

	static_assert(is_input_iterator<TABLE1>::value, "TABLE1::Iterator is not input_iterator");
	static_assert(is_input_iterator<TABLE2>::value, "TABLE2::Iterator is not input_iterator");

public:
	using difference_type = typename it_traits<TABLE1>::difference_type;
	using value_type = Pair;
	using pointer = const value_type *;
	using reference = value_type &;
	using iterator_category = std::input_iterator_tag;

	template<bool B>
	using bool_constant = std::integral_constant<bool, B>;

public:
	template<bool B>
	DualIterator(const TABLE1 &table1, TABLE2 const &table2, bool_constant<B> tag) :
					it1_(table1, tag),
					it2_(table2, tag){}

	DualIterator(const TABLE1 &table1, TABLE2 const &table2, const StringRef &key) :
					it1_(table1, key),
					it2_(table2, key){}

	DualIterator &operator++();

	const Pair &operator*() const;

	bool operator==(const DualIterator &other) const{
		return it1_ == other.it1_ && it2_ == other.it2_;
	}

public:
	bool operator!=(const DualIterator &other) const{
		return ! operator==(other);
	}

	const Pair *operator ->() const{
		return & operator*();
	}

private:
	multiiterator_impl_::IteratorPair<TABLE1>	it1_;
	multiiterator_impl_::IteratorPair<TABLE2>	it2_;
};

} // namespace multi
} // namespace


#include "dualiterator.h.cc"


#endif

