#ifndef _INTRUSIVE_LINK_LIST_LIST_H
#define _INTRUSIVE_LINK_LIST_LIST_H

#include "ilist.h"

#include "listcounter.h"

#include "pmallocator.h"

namespace hm4{


class IntrusiveLinkList{
public:
	using Allocator		= MyAllocator::PMAllocator;

	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

public:
	class iterator;

public:
	IntrusiveLinkList(Allocator &allocator);
	IntrusiveLinkList(IntrusiveLinkList &&other);
	~IntrusiveLinkList(){
		clear();
	}

public:
	bool clear();

	bool erase(std::string_view const key);

	iterator insert(	std::string_view key, std::string_view val,
				uint32_t expires = 0, uint32_t created = 0);

	iterator insert(Pair const &src);

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
	template<class Bytes, class Create>
	iterator insert_(std::string_view const key, Bytes bytes, Create create);

	void clear_();

	struct NodeLocator;

	NodeLocator locate_(std::string_view const key);
	const Node *locateNode_(std::string_view const key, bool exact) const;

	void eraseNode_(NodeLocator const &loc);
};

// ==============================

class IntrusiveLinkList::iterator {
public:
	constexpr iterator(const Node *node) : node_(node){}

public:
	using difference_type = IntrusiveLinkList::difference_type;
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
inline auto IntrusiveLinkList::find(std::string_view const key, std::bool_constant<B> const exact) const -> iterator{
	return locateNode_(key, exact.value);
}

inline auto IntrusiveLinkList::begin() const -> iterator{
	return head_;
}

inline constexpr auto IntrusiveLinkList::end() -> iterator{
	return nullptr;
}

}

#endif
