#ifndef UNROLLED_LINK_LIST_LIST_H
#define UNROLLED_LINK_LIST_LIST_H

#include "ilist.h"
#include "listcounter.h"
#include "pairvector.h"

namespace hm4{


template<class T_Allocator>
class UnrolledLinkList{
public:
	using Allocator			= T_Allocator;
	using size_type			= config::size_type;
	using difference_type		= config::difference_type;

private:
	using MyPairVector		= PairVector<Allocator, 1024>;

public:
	class iterator;

public:
	UnrolledLinkList(Allocator &allocator);
	UnrolledLinkList(UnrolledLinkList &&other);
	~UnrolledLinkList(){
		clear();
	}

	void print() const;

public:
	bool clear();

	bool erase_(std::string_view const key);

	template<class PFactory>
	iterator insertF(PFactory &factory);

	auto size() const{
		return lc_.size();
	}

	auto mutable_size() const{
		return size();
	}

	void mutable_notify(const Pair *, PairFactoryMutableNotifyMessage const &msg){
		lc_.upd(msg.bytes_old, msg.bytes_new);
	}

	auto bytes() const{
		return lc_.bytes();
	}

	auto mutable_bytes() const{
		return bytes();
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

	void zeroing_();

	iterator fix_iterator_(const Node *node, typename MyPairVector::iterator           it) const;
	iterator fix_iterator_(const Node *node, typename MyPairVector::const_ptr_iterator it) const;

	struct NodeLocator;

	template<typename HPairHKey>
	NodeLocator locate_(HPairHKey const hkey, std::string_view const key);
};

// ==============================

template<class T_Allocator>
class UnrolledLinkList<T_Allocator>::iterator {
public:
	using MyPairVector		= UnrolledLinkList::MyPairVector;
	using MyPairVectorIterator	= typename MyPairVector::iterator;
	using MyPairVectorIteratorC	= typename MyPairVector::const_ptr_iterator;

	constexpr iterator() = default;

	explicit constexpr iterator(const Node *node, MyPairVectorIterator it) : node_(node), it_(it){}
	explicit constexpr iterator(const Node *node, MyPairVectorIteratorC it) :
					iterator{
						node,
						MyPairVectorIterator{it}
					}{}

public:
	using difference_type = UnrolledLinkList::difference_type;
	using value_type = const Pair;
	using pointer = value_type *;
	using reference = value_type &;
	using iterator_category = std::forward_iterator_tag;

public:
	iterator &operator++();
	reference operator*() const;

public:
	bool operator==(iterator const &other) const{
		if (node_ != other.node_)
			return false;

		return node_ ? it_ == other.it_ : true;
	}

	bool operator!=(iterator const &other) const{
		return ! operator==(other);
	}

	pointer operator ->() const{
		return & operator*();
	}

private:
	const Node		*node_	= nullptr;
	MyPairVectorIterator	it_{};
};

// ==============================

template<class T_Allocator>
constexpr auto UnrolledLinkList<T_Allocator>::end() -> iterator{
	return {};
}

}

#endif
