namespace hm4{
namespace multi{

#include <stdexcept>	// logic_error

// faster solution with 1 comparison.
template <class TABLE1, class TABLE2>
auto DualIterator<TABLE1, TABLE2>::operator++() -> DualIterator &{
	const Pair *pair1 = it1_.dptr();
	const Pair *pair2 = it2_.dptr();

	if (pair1 == nullptr || pair2 == nullptr){
		if (pair1)
			++it1_;

		if (pair2)
			++it2_;

		return *this;
	}

	auto const cmp = pair1->cmp(pair2->getKey());

	if (cmp <= 0)
		++it1_;

	if (cmp >= 0)
		++it2_;

	return *this;
}

template <class TABLE1, class TABLE2>
const Pair &DualIterator<TABLE1, TABLE2>::operator*() const{
	const Pair *pair1 = it1_.dptr();
	const Pair *pair2 = it2_.dptr();

	if (pair1 == nullptr || pair2 == nullptr){
		if (pair1)
			return *pair1;

		if (pair2)
			return *pair2;

		// welcome to undefined behavour
		throw std::logic_error{ "Dereference of end iterator" };
	}

	int const cmp = pair1->cmp(pair2->getKey());

	// return smaller or first
	return cmp <= 0 ? *pair1 : *pair2;
}


} // namespace multi
} // namespace

