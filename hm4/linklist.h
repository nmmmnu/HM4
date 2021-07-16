#ifndef _LINK_LIST_LIST_H
#define _LINK_LIST_LIST_H

#include "ilist.h"

#include "listcounter.h"

#include "pmallocator.h"

namespace hm4{


class LinkList{
public:
	using Allocator		= MyAllocator::PMAllocator;

	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

public:
	class iterator;

public:
	LinkList(Allocator &allocator);
	LinkList(LinkList &&other);
	~LinkList(){
		clear();
	}

public:
	bool clear();

	bool erase(std::string_view const key);

	iterator insert(	std::string_view key, std::string_view val,
			uint32_t expires = 0, uint32_t created = 0);

	iterator insert(Pair const &src);

	iterator insert(typename Pair::smart_ptr::type<Allocator> &&newdata);

	auto size() const{
		return lc_.size();
	}

	auto bytes() const{
		return lc_.bytes();
	}

	const Allocator &getAllocator() const{
		return *allocator_;
	}

	Allocator &getAllocator(){
		return *allocator_;
	}

public:
	template<bool B>
	iterator find(std::string_view const key, std::bool_constant<B> exact) const;

	iterator begin() const;
	static constexpr iterator end();

private:
	struct Node;

	Node		*head_;

	ListCounter	lc_;

	Allocator	*allocator_;

private:
	void deallocate_(Node *node);

	void clear_();

	struct NodeLocator;

	NodeLocator locate_(std::string_view const key);
	const Node *locateNode_(std::string_view const key, bool exact) const;
};

// ==============================

class LinkList::iterator {
public:
	constexpr iterator(const Node *node) : node_(node){}

public:
	using difference_type = LinkList::difference_type;
	using value_type = const Pair;
	using pointer = value_type *;
	using reference = value_type &;
	using iterator_category = std::forward_iterator_tag;

public:
	iterator &operator++();
	reference operator*() const;

public:
	bool operator==(iterator const &other) const{
		return node_ == other.node_;
	}

	bool operator!=(iterator const &other) const{
		return ! operator==(other);
	}

	pointer operator ->() const{
		return & operator*();
	}

private:
	const Node	*node_;
};

// ==============================

template<bool B>
inline auto LinkList::find(std::string_view const key, std::bool_constant<B> const exact) const -> iterator{
	return locateNode_(key, exact.value);
}

inline auto LinkList::begin() const -> iterator{
	return head_;
}

inline constexpr auto LinkList::end() -> iterator{
	return nullptr;
}

inline auto LinkList::insert(
		std::string_view key, std::string_view val,
		uint32_t expires, uint32_t created) -> iterator{

	return hm4::insert(*this, key, val, expires, created);
}

inline auto LinkList::insert(Pair const &src) -> iterator{
	return hm4::insert(*this, src);
}



}

#endif
