//#include <algorithm>	// std::equal

namespace hm4{
namespace multiiterator{

template <class TABLE>
template <class CONTAINER, typename T>
CollectionIterator<TABLE>::CollectionIterator(const CONTAINER &list, const T &iteratorParam, std::nullptr_t){
	it_.reserve(list.size());

	// CONTAINER is responsible for ordering the tables,
	// in correct (probably reverse) order.
	for(const auto &table : list)
		it_.push_back({ table, iteratorParam });

	tmp_index_pp.reserve(list.size());
	tmp_index_de.reserve(list.size());
}

template <class TABLE>
auto CollectionIterator<TABLE>::operator++() -> CollectionIterator &{
	const Pair &pair = operator*();

	if ( ! pair ){
		// notice, there is no increment here !!!
		ended_ = true;
		return *this;
	}

	// step 2: increase all duplicates from the index
	for(const auto index : tmp_index_pp)
		++it_[index];

	tmp_pair = nullptr;

	return *this;
}

template <class TABLE>
const Pair &CollectionIterator<TABLE>::dereference_() const{
	const Pair *cpair = nullptr;

	tmp_index_de.clear();

	// step 1: find first minimal add other minimals into index
	for(size_type i = 0; i < it_.size(); ++i){
		const Pair &pair = *it_[i];

		// skip if is null
		if ( ! pair )
			continue;

		// initialize
		if (cpair == nullptr){
			tmp_index_de.clear();
			tmp_index_de.push_back(i);
			cpair = &pair;

			continue;
		}

		int const cmp = pair.cmp(*cpair);

		// if is smaller, start over
		if (cmp < 0){
			tmp_index_de.clear();
			tmp_index_de.push_back(i);
			cpair = &pair;

			continue;
		}

		// same, add to list.
		if (cmp == 0){
			tmp_index_de.push_back(i);

			// if newer, swap
			if ( pair.cmpTime(*cpair) > 0 )
				cpair = &pair;
		}
	}

	// SETTING MUTABLE VARIABLES
	if (cpair){
		tmp_pair = cpair;
		std::swap(tmp_index_pp, tmp_index_de);

		return *tmp_pair;
	}else{
		tmp_pair = cpair;

		return Pair::zero();
	}
}

} // namespace multitableiterator
} // namespace

