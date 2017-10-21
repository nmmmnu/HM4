namespace hm4{
namespace multi{


template <class CONTAINER>
const Pair *CollectionList<CONTAINER>::operator[](const StringRef &key) const{
	assert(!key.empty());

	// CONTAINER is responsible for ordering the tables,
	// in correct (probably reverse) order.

	// CONTAINER is responsible for providing goof const Pair &.

	for(const auto &table : container_ ){
		if (const Pair *pair = table[key])
			return pair;
	}

	return nullptr;
}

// ===================================

template <class CONTAINER>
size_t CollectionList<CONTAINER>::bytes() const{
	size_t result = 0;

	for(const auto &table : container_ )
		result += table.bytes();

	return result;
}

template <class CONTAINER>
auto CollectionList<CONTAINER>::sizeEstimated_(bool const estimated) const -> size_type{
	size_type result = 0;

	for(const auto &table : container_ )
		result += table.size(estimated);

	return result;
}

} // namespace multi
} // namespace

