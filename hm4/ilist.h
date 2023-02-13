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

namespace PairFactory{
	struct MutableNotifyMessage{
		size_t bytes_old = 0;
		size_t bytes_new = 0;
	};

	struct Normal{
		std::string_view key;
		std::string_view val;
		uint32_t expires = 0;
		uint32_t created = 0;

		[[nodiscard]]
		constexpr auto getKey() const noexcept{
			return key;
		}

		template<class List>
		[[nodiscard]]
		bool operator()(Pair *hint, List &list) const noexcept{
			MutableNotifyMessage msg;
			msg.bytes_old = hint->bytes();
			msg.bytes_new = Pair::bytes(key, val);

			if (msg.bytes_new <= msg.bytes_old){
				Pair::createInRawMemory<0,1>(hint, key, val, expires, created);

				list.mutable_notify(hint, msg);

				return true;
			}

			return false;
		}

		template<class Allocator>
		[[nodiscard]]
		auto operator()(Allocator &allocator) const noexcept{
			return Pair::smart_ptr::create(allocator, key, val, expires, created);
		}
	};

	struct NormalExpiresOnly : Normal{
		template<class List>
		[[nodiscard]]
		bool operator()(Pair *hint, List &list) const noexcept{
			Pair::createInRawMemory<0,0>(hint, key, val, expires, created);

			MutableNotifyMessage msg;
			list.mutable_notify(hint, msg);

			return true;
		}

		using Normal::operator();
	};

	struct Tombstone{
		std::string_view key;

		[[nodiscard]]
		constexpr auto getKey() const noexcept{
			return key;
		}

		template<class List>
		[[nodiscard]]
		bool operator()(Pair *hint, List &list) const noexcept{
			MutableNotifyMessage msg;
			msg.bytes_old = hint->bytes();
			msg.bytes_new = Pair::bytes(key, Pair::TOMBSTONE);

			Pair::createInRawMemory<0,1>(hint, key, Pair::TOMBSTONE, 0, 0);

			list.mutable_notify(hint, msg);

			return true;
		}

		template<class Allocator>
		[[nodiscard]]
		auto operator()(Allocator &allocator) const noexcept{
			return Pair::smart_ptr::create(allocator, key, Pair::TOMBSTONE);
		}
	};

	struct Clone{
		const Pair *src;

		[[nodiscard]]
		auto getKey() const noexcept{
			return src->getKey();
		}

		template<class List>
		[[nodiscard]]
		bool operator()(Pair *hint, List &list) const noexcept{
			MutableNotifyMessage msg;
			msg.bytes_old = hint->bytes();
			msg.bytes_new =  src->bytes();

			if (msg.bytes_new <= msg.bytes_old){
				Pair::cloneInRawMemory(hint, *src);

				list.mutable_notify(hint, msg);

				return true;
			}

			return false;
		}

		template<class Allocator>
		[[nodiscard]]
		auto operator()(Allocator &allocator) const noexcept{
			return Pair::smart_ptr::clone(allocator, src);
		}
	};

	constexpr auto factory(
			std::string_view key, std::string_view val,
			uint32_t const expires = 0,
			uint32_t const created = 0){

		return PairFactory::Normal{ key, val, expires, created };
	}

	constexpr auto factory(
			uint32_t const expires,
			std::string_view key, std::string_view val,
			uint32_t const created = 0){
		return PairFactory::NormalExpiresOnly{ key, val, expires, created };
	}

	constexpr auto factory(std::string_view key){
		return PairFactory::Tombstone{ key };
	}

	constexpr auto factory(Pair const &src){
		return PairFactory::Clone{ & src };
	}

} // namespace PairFactory

using PairFactoryMutableNotifyMessage = PairFactory::MutableNotifyMessage;

// ==============================

template<class List>
bool erase(List &list, std::string_view const key){
	return list.erase_(key);
}

template<class List, typename ...Args>
auto insert(List &list, Args &&...args){
	return list.insertLazyPair_(
		PairFactory::factory(std::forward<Args>(args)...)
	);
}

template<bool AllowHints, class List>
constexpr bool canInsertHint(const List &list, const Pair *pair){
	if constexpr(AllowHints)
		return list.getAllocator().owns(pair);

	return false;
}

template<bool AllowHints, class List>
constexpr bool canInsertHint(const List &list, const Pair *pair, size_t val_size){
	if constexpr(AllowHints)
		return list.getAllocator().owns(pair) && pair->getVal().size() >= val_size;

	return false;
}

template<class List, typename ...Args>
constexpr bool proceedInsertHint(List &list, const Pair *pair, Args &&...args){
	// Pair is in the memlist and it is safe to be overwitten.
	// the create time is not updated, but this is not that important,
	// since the Pair is not yet flushed.
	Pair *m_pair = const_cast<Pair *>(pair);

	auto factory = PairFactory::factory(std::forward<Args>(args)...);

	if (factory(m_pair, list))
		return true;
	else
		return false;
}

template<bool AllowHints = true, class List, typename ...Args>
constexpr bool tryInsertHint(List &list, const Pair *pair, Args &&...args){
	if constexpr(AllowHints){
		if (canInsertHint<AllowHints>(list, pair))
			return proceedInsertHint(list, pair, std::forward<Args>(args)...);
	}

	return false;
}

template<bool AllowHints = true, class List, typename ...Args>
void insertHint(List &list, const Pair *pair, Args &&...args){
	if (tryInsertHint<AllowHints>(list, pair, std::forward<Args>(args)...) == false)
		insert(list, std::forward<Args>(args)...);
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

