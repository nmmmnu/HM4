namespace hm4{
namespace multiiterator{


// faster solution with 1 comparison.
template <class TABLE1, class TABLE2>
auto DualIterator<TABLE1, TABLE2>::operator++() -> DualIterator &{
	const Pair &pair1 = *it1_;
	const Pair &pair2 = *it2_;

	const auto cmp = pair1.cmp(pair2);

	if (cmp <= 0)
		++it1_;

	if (cmp >= 0)
		++it2_;

	return *this;
}

template <class TABLE1, class TABLE2>
const Pair &DualIterator<TABLE1, TABLE2>::operator*() const{
	const Pair &pair1 = *it1_;
	const Pair &pair2 = *it2_;

	int const cmp = pair1.cmp(pair2);

	if (cmp != 0){
		// return smaller
		return  cmp < 0 ? pair1 : pair2;
	}else{
		// return newer
		return pair1.cmpTime(pair2) > 0 ? pair1 : pair2;
	}
}


} // namespace multitableiterator
} // namespace

