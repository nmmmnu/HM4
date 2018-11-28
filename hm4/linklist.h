#ifndef _LINK_LIST_LIST_H
#define _LINK_LIST_LIST_H

#include "ilist.h"

namespace hm4{


class LinkList{
public:
	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

public:
	class Iterator;

public:
	LinkList();
	LinkList(LinkList &&other);
	~LinkList(){
		clear();
	}

public:
	bool clear();

	const Pair *operator[](const StringRef &key) const;
	bool erase(StringRef const &key);

	bool insert(OPair &&data);

	size_type size() const{
		return dataCount_;
	}

	size_t bytes() const{
		return dataSize_;
	}

public:
	Iterator lowerBound(StringRef const &key) const;

	Iterator begin() const;
	static constexpr Iterator end();

private:
	struct Node;

	Node		*head_;

	size_type	dataCount_;
	size_t		dataSize_;

private:
	void clear_();

	struct NodeLocator;

	NodeLocator locate_(StringRef const &key);
	const Node *locateNode_(StringRef const &key, bool exact) const;
};

// ==============================

class LinkList::Iterator {
protected:
	friend class LinkList;

	constexpr Iterator(const Node *node) : node_(node){}

public:
	constexpr Iterator(Iterator const &other) = default;
	constexpr Iterator(Iterator &&other) = default;

public:
	using difference_type = LinkList::difference_type;
	using value_type = const Pair;
	using pointer = value_type *;
	using reference = value_type &;
	using iterator_category = std::forward_iterator_tag;

public:
	Iterator &operator++();
	reference operator*() const;

public:
	bool operator==(Iterator const &other) const{
		return node_ == other.node_;
	}

	bool operator!=(Iterator const &other) const{
		return ! operator==(other);
	}

	pointer operator ->() const{
		return & operator*();
	}

private:
	const Node	*node_;
};

// ==============================

inline auto LinkList::lowerBound(const StringRef &key) const -> Iterator{
	if (key.empty())
		return begin();

	return locateNode_(key, false);
}

inline auto LinkList::begin() const -> Iterator{
	return head_;
}

inline constexpr auto LinkList::end() -> Iterator{
	return nullptr;
}


}

#endif
