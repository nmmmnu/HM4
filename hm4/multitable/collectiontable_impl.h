namespace hm4{
namespace multitable{


template <class CONTAINER>
const Pair &CollectionTable<CONTAINER>::operator[](const StringRef &key) const{
	// precondition
	assert(!key.empty());
	// eo precondition

	// CONTAINER is responsible for ordering the tables,
	// in correct (probably reverse) order.

	// CONTAINER is responsible for providing goof const Pair &.

	for(const auto &table : container_ ){
		if (const Pair &pair = table[key])
			return pair;
	}

	return Pair::zero();
}

// ===================================

template <class CONTAINER>
size_t CollectionTable<CONTAINER>::bytes() const{
	size_t result = 0;

	for(const auto &table : container_ )
		result += table.bytes();

	return result;
}

template <class CONTAINER>
auto CollectionTable<CONTAINER>::sizeEstimated_() const -> size_type{
	size_type result = 0;

	for(const auto &table : container_ )
		result += table.size();

	return result;
}

template <class CONTAINER>
auto CollectionTable<CONTAINER>::sizeReal_() const -> size_type{
	// Slooooow....
	size_type count = 0;
	for(const auto &p : *this){
		(void) p;
		++count;
	}

	return count;
}


} // multitable
} // namespace

