#ifndef MY_ILIST_H_
#define MY_ILIST_H_

#include <cstdint>
#include <iterator>	// std::distance

#include "pair.h"

#include "logger.h"

namespace hm4{

namespace config{
	using size_type		= size_t;
	using difference_type	= ptrdiff_t;

	static_assert(sizeof(size_type      ) == 8, "You need 64bit system!");
	static_assert(sizeof(difference_type) == 8, "You need 64bit system!");

	constexpr size_type	LIST_PRINT_COUNT		= 10;

	constexpr bool		LIST_CHECK_PAIR_FOR_REPLACE	= true;

	constexpr auto		MAX_INTERNAL_NODE_SIZE		= 528; // skiplist
}

template<size_t size>
constexpr size_t checkInternalNodeSize(){
	static_assert(config::MAX_INTERNAL_NODE_SIZE >= size);

	return size;
}

struct InsertResult{
	enum class Status{
		INSERTED		,
		DELETED			,
		UPDATED_IN_PLACE	,
		REPLACED		,
		SKIP_INSERTED		,
		SKIP_DELETED		,
		ERROR_NO_MEMORY		,
		ERROR_INVALID		,
		ERROR
	};

	bool		ok;
	Status		status;
	const Pair	*pair	= nullptr;

	constexpr static auto INSERTED		=  Status::INSERTED		;
	constexpr static auto DELETED		=  Status::DELETED		;
	constexpr static auto UPDATED_IN_PLACE	=  Status::UPDATED_IN_PLACE	;
	constexpr static auto REPLACED		=  Status::REPLACED		;
	constexpr static auto SKIP_INSERTED	=  Status::SKIP_INSERTED	;
	constexpr static auto SKIP_DELETED	=  Status::SKIP_DELETED		;
	constexpr static auto ERROR_NO_MEMORY	=  Status::ERROR_NO_MEMORY	;
	constexpr static auto ERROR_INVALID	=  Status::ERROR_INVALID	;
	constexpr static auto ERROR		=  Status::ERROR		;

	[[nodiscard]]
	constexpr static auto inserted(const Pair *pair){
		return InsertResult{ true, InsertResult::INSERTED, pair };
	}

	[[nodiscard]]
	constexpr static auto deleted(){
		return InsertResult{ true, InsertResult::DELETED };
	}

	[[nodiscard]]
	constexpr static auto updatedInPlace(Pair *pair){
		return InsertResult{ true, InsertResult::UPDATED_IN_PLACE, pair };
	}

	[[nodiscard]]
	constexpr static auto replaced(Pair *pair){
		return InsertResult{ true, InsertResult::REPLACED, pair };
	}

	[[nodiscard]]
	constexpr static auto skipInserted(){
		return InsertResult{ true, InsertResult::SKIP_INSERTED };
	}

	[[nodiscard]]
	constexpr static auto skipDeleted(){
		return InsertResult{ false, InsertResult::SKIP_DELETED };
	}

	[[nodiscard]]
	constexpr static auto errorNoMemory(){
		return InsertResult{ false, InsertResult::ERROR_NO_MEMORY };
	}

	[[nodiscard]]
	constexpr static auto errorInvalid(){
		return InsertResult{ false, InsertResult::ERROR_INVALID };
	}

	[[nodiscard]]
	constexpr static auto error(){
		return InsertResult{ false, InsertResult::ERROR };
	}
};

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
	struct conf_estimated_size : std::false_type{};

	template<class List>
	struct conf_estimated_size<List, std::void_t<decltype(List::conf_estimated_size)> >: std::bool_constant<List::conf_estimated_size>{};
}

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
	using tag = ilist_impl_::conf_estimated_size<List>;

	return size(list, tag{});
}

// ==============================

// template<class List>
// bool empty(List const &list){
// 	return size(list, std::false_type{}) == 0;
// }

using std::empty;

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
InsertResult erase(List &list, std::string_view const key){
	return list.erase_(key);
}

template<class List, class PairFactory>
InsertResult insert(List &list, PairFactory &factory) noexcept{
	static_assert(!std::is_same_v<PairFactory, Pair>);
	static_assert(!std::is_same_v<PairFactory, std::basic_string_view<char, std::char_traits<char> > const>);

	return list.insertF(factory);
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

// used in DualList only
template<class List>
auto insertTS(List &list, std::string_view const key) noexcept{
	return insertF<PairFactory::Tombstone>(list, key);
}

template<class List>
auto insert(List &list, Pair const &src) noexcept{
	return insertF<PairFactory::Clone>(list, src);
}

// if not present, insert(list, Factory) will be called.
template<class List>
auto insert(List &list, Pair &src) noexcept{
	Pair const &srcc = src;
	return insert(list, srcc);
}

// ==============================

template<class Allocator>
constexpr bool canInsertHintAllocator(Allocator const &allocator, const Pair *pair){
	return allocator.owns(pair);
}

template<class List>
constexpr bool canInsertHintList(List const &list, const Pair *pair){
	return canInsertHintAllocator(list.getAllocator(), pair);
}

template<class List>
constexpr bool canInsertHintValSize(List const &list, const Pair *pair, size_t val_size){
	return
		canInsertHintList(list, pair) &&
		pair->getVal().size() >= val_size
	;
}

template<class Allocator, class PairFactory>
constexpr bool canInsertHintAllocatorF(Allocator const &allocator, const Pair *pair, PairFactory const &factory){
	return
		canInsertHintAllocator(allocator, pair) &&
		pair->bytes() >= factory.bytes()
	;
}

// ==============================

// proceedInsertHint:
// 	Pair is in the memlist,
// 	data size is already checked and it is safe to be overwitten.
//
// 	The create time is not updated, but this is not that important,
// 	since the Pair is not yet flushed.

template<class PairFactory>
constexpr void proceedInsertHint_skipMutableNotify(Pair *pair, PairFactory &factory){
	factory.createHint(pair);

	logger<Logger::DEBUG>() << "inserting hint for key" << factory.getKey();
}

template<class List, class PairFactory>
constexpr void proceedInsertHint(List &list, Pair *pair, PairFactory &factory){
	auto const bytes_old = pair->bytes();

	proceedInsertHint_skipMutableNotify(pair, factory);

	PairFactoryMutableNotifyMessage msg{ pair, bytes_old, pair->bytes() };

	msg.bytes_new = pair->bytes();

	list.mutable_notify(msg);
}

template<class PairFactory, class List, typename ...Args>
auto proceedInsertHintF(List &list, Pair *pair, Args &&...args){
	PairFactory factory{ std::forward<Args>(args)... };

	return proceedInsertHint(list, pair, factory);
}

template<class VPairFactory, class List, typename ...Args>
auto proceedInsertHintV(List &list, Pair *pair, Args &&...args){
	using VBase = PairFactory::IFactory;

	static_assert(std::is_base_of_v<VBase, VPairFactory>, "VPairFactory must derive from PairFactory::IFactory");

	VPairFactory factory{ std::forward<Args>(args)... };

	// lost the type
	VBase &vbase = factory;

	return proceedInsertHint(list, pair, vbase);
}

// ==============================

// insertHint - standard insert, but uses hint, if possible

template<class List, class PairFactory>
void insertHint(List &list, const Pair *pair, PairFactory &factory){
	if (canInsertHintAllocatorF(list.getAllocator(), pair, factory))
		proceedInsertHint(list, const_cast<Pair *>(pair), factory);
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

template<bool CheckOK = true, typename List, typename Predicate>
auto getPair_(List const &list, std::string_view key, Predicate p){
	auto it = list.find(key, std::true_type{});

	if constexpr(CheckOK){
		bool const b = it != std::end(list) && it->isOK();

		return p(b, it);
	}else{
		bool const b = it != std::end(list);

		return p(b, it);
	}
}

template<typename List>
auto getPairOK(List const &list, std::string_view key){
	return getPair_(list, key, [](bool b, auto ){
		return b;
	});
}

template<typename List>
auto getPairVal(List const &list, std::string_view key){
	return getPair_(list, key, [](bool b, auto it){
		return b ? it->getVal() : "";
	});
}

template<bool B, typename List>
auto getPairPtr_(List const &list, std::string_view key){
	return getPair_<B>(list, key, [](bool b, auto it){
		return b ? & *it : nullptr;
	});
}

template<typename List>
auto getPairPtr(List const &list, std::string_view key){
	return getPairPtr_<1>(list, key);
}

template<typename List>
auto getPairPtrWithSize(List const &list, std::string_view key, size_t size){
	return hm4::getPair_(list, key, [size](bool b, auto it) -> const hm4::Pair *{
		if (b && it->getVal().size() == size)
			return & *it;
		else
			return nullptr;
	});
}

template<typename List>
auto getPairPtrNC(List const &list, std::string_view key){
	return getPairPtr_<0>(list, key);
}

// ==============================

#if 0

template<typename List, typename Predicate>
auto getPairByPrefix_(List const &list, std::string_view key, Predicate p){
	auto it = list.find(key, std::false_type{});

	bool const b = it != std::end(list) && it->isOK() && same_prefix(key, it->getKey());

	return p(b, it);
}

template<typename List>
auto getPairOKByPrefix(List const &list, std::string_view key){
	return getPairByPrefix_(list, key, [](bool b, auto ){
		return b;
	});
}

template<typename List>
auto getPairValByPrefix(List const &list, std::string_view key){
	return getPairByPrefix_(list, key, [](bool b, auto it){
		return b ? it->getVal() : "";
	});
}

template<typename List>
auto getPairPtrByPrefix(List const &list, std::string_view key){
	return getPairByPrefix_(list, key, [](bool b, auto it){
		return b ? & *it : nullptr;
	});
}

#endif

} // namespace

#endif

