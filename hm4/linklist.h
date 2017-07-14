#ifndef _LINK_LIST_LIST_H
#define _LINK_LIST_LIST_H

#include "ilist.h"

namespace hm4{


class LinkList : public IMutableList<LinkList>{
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

	const Pair &operator[](const StringRef &key) const;
	bool erase(const StringRef &key);

	size_type size(bool const = true) const{
		return dataCount_;
	}

	size_t bytes() const{
		return dataSize_;
	}

public:
	Iterator lowerBound(const StringRef &key) const;

	Iterator begin() const;
	Iterator end() const;

private:
	friend class IMutableList<LinkList>;

	template <class UPAIR>
	bool insertT_(UPAIR &&data);

private:
	struct Node;

	Node		*head_;

	size_type	dataCount_;
	size_t		dataSize_;

private:
	void clear_();

	const Node *locate_(const StringRef &key, bool exact) const;
};

// ==============================

class LinkList::Iterator {
protected:
	friend class LinkList;

	Iterator(const Node *node) : node_(node){}

public:
	Iterator &operator++();
	const Pair &operator*() const;
	bool operator==(const Iterator &other) const;

public:
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

inline LinkList::Iterator LinkList::lowerBound(const StringRef &key) const{
	if (key.empty())
		return begin();

	const Node *node = locate_(key, false);
	return Iterator(node);
}

inline LinkList::Iterator LinkList::begin() const{
	return Iterator(head_);
}

inline LinkList::Iterator LinkList::end() const{
	return Iterator(nullptr);
}


}

#endif
