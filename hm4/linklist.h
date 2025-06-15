#ifndef _LINK_LIST_LIST_H
#define _LINK_LIST_LIST_H

#include "ilist.h"

#include "listcounter.h"

namespace hm4{


template<class T_Allocator>
class LinkList{
public:
	using Allocator		= T_Allocator;
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

	InsertResult erase_(std::string_view const key);

	template<class PFactory>
	InsertResult insertF(PFactory &factory);

	auto size() const{
		return lc_.size();
	}

	auto empty() const{
		return size() == 0;
	}

	auto const &mutable_list() const{
		return *this;
	}

	void mutable_notify(PairFactoryMutableNotifyMessage const &msg){
		lc_.upd(msg.bytes_old, msg.bytes_new);
	}

	auto bytes() const{
		return lc_.bytes();
	}

	constexpr static void crontab(){
	}

	const Allocator &getAllocator() const{
		return *allocator_;
	}

	Allocator &getAllocator(){
		return *allocator_;
	}

public:
	iterator    find     (std::string_view const key) const;
	const Pair *findExact(std::string_view const key) const;

	iterator begin() const;
	static constexpr iterator end();

private:
	struct Node;

	Node		*head_;

	ListCounter	lc_;

	Allocator	*allocator_;

public:
	static size_t const INTERNAL_NODE_SIZE;

private:
	template<bool B>
	const Node *find_(std::string_view const key) const;

private:
	void deallocate_(Node *node);

	void clear_();

	struct NodeLocator;

	NodeLocator locate_(std::string_view const key);
};

// ==============================

template<class T_Allocator>
class LinkList<T_Allocator>::iterator {
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

template<class T_Allocator>
inline auto LinkList<T_Allocator>::begin() const -> iterator{
	return head_;
}

template<class T_Allocator>
constexpr auto LinkList<T_Allocator>::end() -> iterator{
	return nullptr;
}

}

#endif
