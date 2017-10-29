#ifndef _LINK_LIST_LIST_H
#define _LINK_LIST_LIST_H

#include "ilist.h"

namespace hm4{


class LinkList : public IList<LinkList, true>{
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
	bool erase(const StringRef &key);

	bool insert(OPair &&data);

	size_type size(bool const = false) const{
		return dataCount_;
	}

	size_t bytes() const{
		return dataSize_;
	}

public:
	Iterator lowerBound(const StringRef &key) const;

	Iterator begin() const;
	static constexpr Iterator end();

private:
	struct Node;

	Node		*head_;

	size_type	dataCount_;
	size_t		dataSize_;

private:
	void clear_();

	const Node *locate_(const StringRef &key, bool exact) const;

	static int ocmp__(const OPair &p, const StringRef &key){
		return p.cmp(key);
	}
};

// ==============================

class LinkList::Iterator {
protected:
	friend class LinkList;

	constexpr Iterator(const Node *node) : node_(node){}

public:
	Iterator &operator++();
	const Pair &operator*() const;

public:
	bool operator==(const Iterator &other) const{
		return node_ == other.node_;
	}

	bool operator!=(const Iterator &other) const{
		return ! operator==(other);
	}

	const Pair *operator ->() const{
		return & operator*();
	}

private:
	const Node	*node_;
};

// ==============================

inline auto LinkList::lowerBound(const StringRef &key) const -> Iterator{
	if (key.empty())
		return begin();

	const Node *node = locate_(key, false);
	return Iterator(node);
}

inline auto LinkList::begin() const -> Iterator{
	return Iterator(head_);
}

inline constexpr auto LinkList::end() -> Iterator{
	return Iterator(nullptr);
}


}

#endif
