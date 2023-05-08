#ifndef MY_LIST_H_
#define MY_LIST_H_

#include <cstdint>
#include <iterator>	// std::distance

#include "pair.h"

namespace hm4{

namespace config{
	using size_type		= size_t;
	using difference_type	= ptrdiff_t;

	static_assert(sizeof(size_type      ) == 8, "You need 64bit system!");
	static_assert(sizeof(difference_type) == 8, "You need 64bit system!");

	constexpr size_type	LIST_PRINT_COUNT		= 10;

	constexpr bool		LIST_CHECK_PAIR_FOR_REPLACE	= true;
}

// ==============================

template<class List>
void print(List const &list, typename List::size_type count = config::LIST_PRINT_COUNT){
	for(auto const &p : list){
		print(p);

		if (--count == 0)
			return;
	}
}

// ==============================

namespace ilist_impl_{
	template<class List, class = void>
	struct size_estimated : std::false_type{};

	template<class List>
	struct size_estimated<List, std::void_t<typename List::estimated_size> >: std::true_type{};
} // namespace ilist_impl

// ==============================

template<class List>
auto size(List const &list, std::false_type){
	return list.size();
}

template<class List>
auto size(List const &list, std::true_type){
	return narrow<typename List::size_type>(
		std::distance(std::begin(list), std::end(list))
	);
}

template<class List>
auto size(List const &list){
	using size_estimated = ilist_impl_::size_estimated<List>;

	return size(list, size_estimated{});
}

// ==============================

template<class List>
bool empty(List const &list){
	return size(list, std::false_type{}) == 0;
}

// ==============================

[[nodiscard]]
inline bool isValidForReplace(uint64_t const neo_created_64, Pair const &old) noexcept{
	return neo_created_64 >= old.getCreated();
}

[[nodiscard]]
inline bool isValidForReplace(uint32_t const neo_created, Pair const &old) noexcept{
	return isValidForReplace(Pair::prepareCreateTimeSimulate(neo_created), old);
}

[[nodiscard]]
inline bool isValidForReplace(Pair const &neo, Pair const &old) noexcept{
	return isValidForReplace(neo.getCreated(), old);
}

// ==============================

using PairFactoryMutableNotifyMessage = PairFactory::MutableNotifyMessage;

// ==============================

template<class List>
bool erase(List &list, std::string_view const key){
	return list.erase_(key);
}

template<class List, class PairFactory>
auto insert(List &list, PairFactory &factory) noexcept{
	static_assert(!std::is_same_v<PairFactory, Pair>);
	static_assert(!std::is_same_v<PairFactory, std::basic_string_view<char, std::char_traits<char> > const>);

	return list.insertLazyPair_(factory);
}

template<class PairFactory, class List, typename ...Args>
auto insertF(List &list, Args &&...args) noexcept {
	PairFactory factory{ std::forward<Args>(args)... };

	return insert(list, factory);
}

template<class VPairFactory, class List, typename ...Args>
auto insertV(List &list, Args &&...args) noexcept{
	using VBase = PairFactory::IFactory;

	static_assert(std::is_base_of_v<VBase, VPairFactory>, "VPairFactory must derive from PairFactory::IFactory");

	VPairFactory factory{ std::forward<Args>(args)... };

	// lost the type
	VBase &vbase = factory;

	return insert(list, vbase);
}

template<class List>
auto insert(List &list, std::string_view const key,
			std::string_view const val,
			uint32_t const expires = 0, uint32_t const created = 0) noexcept{

	return insertF<PairFactory::Normal>(list, key, val, expires, created);
}

template<class List>
auto insert(List &list, std::string_view const key) noexcept{
	return insertF<PairFactory::Tombstone>(list, key);
}

template<class List>
auto insert(List &list, Pair const &src) noexcept{
	return insertF<PairFactory::Clone>(list, src);
}

// if not present, insert(list, Factory) will be called.
template<class List>
auto insert(List &list, Pair &src) noexcept{
	return insertF<PairFactory::Clone>(list, & src);
}

// ==============================

template<class List>
constexpr bool canInsertHint(const List &list, const Pair *pair){
	return list.getAllocator().owns(pair);
}

template<class List>
constexpr bool canInsertHint(const List &list, const Pair *pair, size_t bytes){
	return canInsertHint(list, pair) && pair->getVal().size() >= bytes;
}

// ==============================

template<class List, class PairFactory>
constexpr void proceedInsertHint(List &list, const Pair *pair, PairFactory &factory){
	// Pair is in the memlist,
	// data size is already checked and it is safe to be overwitten.
	//
	// The create time is not updated, but this is not that important,
	// since the Pair is not yet flushed.

	PairFactoryMutableNotifyMessage msg;
	msg.bytes_old = pair->bytes();
	msg.bytes_new = factory.bytes();

	factory.createHint( const_cast<Pair *>(pair) );

	list.mutable_notify(pair, msg);
}

template<class PairFactory, class List, typename ...Args>
auto proceedInsertHintF(List &list, const Pair *pair, Args &&...args){
	PairFactory factory{ std::forward<Args>(args)... };

	return proceedInsertHint(list, pair, factory);
}

template<class VPairFactory, class List, typename ...Args>
auto proceedInsertHintV(List &list, const Pair *pair, Args &&...args){
	using VBase = PairFactory::IFactory;

	static_assert(std::is_base_of_v<VBase, VPairFactory>, "VPairFactory must derive from PairFactory::IFactory");

	VPairFactory factory{ std::forward<Args>(args)... };

	// lost the type
	VBase &vbase = factory;

	return proceedInsertHint(list, pair, vbase);
}

// ==============================

template<class List, class PairFactory>
constexpr bool tryInsertHint(List &list, const Pair *pair, PairFactory &factory){
	if (! canInsertHint(list, pair, factory.bytes()) )
		return false;

	proceedInsertHint(list, pair, factory);

	return true;
}

// ==============================

template<class List, class PairFactory>
void insertHint(List &list, const Pair *pair, PairFactory &factory){
	if (canInsertHint(list, pair, factory.bytes()))
		proceedInsertHint(list, pair, factory);
	else
		insert(list, factory);
}

template<class PairFactory, class List, typename ...Args>
auto insertHintF(List &list, const Pair *pair, Args &&...args){
	PairFactory factory{ std::forward<Args>(args)... };

	return insertHint(list, pair, factory);
}

template<class VPairFactory, class List, typename ...Args>
auto insertHintV(List &list, const Pair *pair, Args &&...args){
	using VBase = PairFactory::IFactory;

	static_assert(std::is_base_of_v<VBase, VPairFactory>, "VPairFactory must derive from PairFactory::IFactory");

	VPairFactory factory{ std::forward<Args>(args)... };

	// lost the type
	VBase &vbase = factory;

	return insertHint(list, pair, vbase);
}

// ==============================

template<class List, bool B>
auto getIterator(List const &list, std::bool_constant<B>){
	if constexpr(B)
		return std::begin(list);
	else
		return std::end(list);
}

template<class List, bool B>
auto getIterator(List const &list, std::string_view const key, std::bool_constant<B> const exact){
	if (key.empty())
		return std::begin(list);
	else
		return list.find(key, exact);
}

// ==============================

template<typename List, typename Predicate>
auto getPair_(List const &list, std::string_view key, Predicate p){
	auto it = list.find(key, std::true_type{});

	bool const b = it != std::end(list) && it->isValid(std::true_type{});

	return p(b, it);
}

template<typename List>
auto getPairVal(List const &list, std::string_view key){
	return getPair_(list, key, [](bool b, auto it){
		return b ? it->getVal() : "";
	});
}

template<typename List>
auto getPairPtr(List const &list, std::string_view key){
	return getPair_(list, key, [](bool b, auto it){
		return b ? & *it : nullptr;
	});
}

template<typename List>
auto getPairOK(List const &list, std::string_view key){
	return getPair_(list, key, [](bool b, auto ){
		return b;
	});
}

} // namespace

#endif

