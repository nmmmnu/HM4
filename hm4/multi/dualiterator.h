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

	template<class T>
	using it_traits_DT = typename it_traits<T>::difference_type;

public:
	using difference_type = it_traits_DT<TABLE1>;
	using value_type = const Pair;
	using pointer = value_type *;
	using reference = value_type &;
	using iterator_category = std::forward_iterator_tag;

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

	reference operator*() const;

	bool operator==(const DualIterator &other) const{
		return it1_ == other.it1_ && it2_ == other.it2_;
	}

public:
	bool operator!=(const DualIterator &other) const{
		return ! operator==(other);
	}

	pointer operator ->() const{
		return & operator*();
	}

private:
	template<class T>
	using IteratorPair = typename multiiterator_impl_::IteratorPair<T>;

	IteratorPair<TABLE1>	it1_;
	IteratorPair<TABLE2>	it2_;

private:
	static_assert(
		std::is_same<it_traits_DT<TABLE1>, it_traits_DT<TABLE2> >::value,
		"TABLE1::Iterator::difference_type must be same as TABLE2::Iterator::difference_type"
	);

	template<class T>
	constexpr static bool is_forward_iterator_v =
		std::is_base_of<
			iterator_category,
			typename it_traits<T>::iterator_category
		>::value
	;

	static_assert(is_forward_iterator_v<TABLE1>, "TABLE1::Iterator is not input_iterator");
	static_assert(is_forward_iterator_v<TABLE2>, "TABLE2::Iterator is not input_iterator");
};

} // namespace multi
} // namespace


#include "dualiterator.h.cc"


#endif

