#ifndef UNROLLED_SKIP_LIST_LIST_H
#define UNROLLED_SKIP_LIST_LIST_H

#include "ilist.h"
#include "listcounter.h"
#include "pairvector.h"

#include <array>

namespace hm4{


template<class T_Allocator>
class UnrolledSkipList{
public:
	using Allocator		= T_Allocator;

	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

	using height_size_type = uint8_t;

private:
	using MyPairVector		= PairVector<Allocator, 1024>;

	//	speculative tests on 10K
	//	skip	12.80
	//
	//	64	14.83
	//	128	14.12
	//	256	12.23
	//	512	11.98
	//	1024	12.36
	//	2048	14.22

	//	speculative tests on 80K
	//	skip	152.32	151.91
	//
	//	256	170.73
	//	512	163.72	161.99*
	//	1024	165.87	172.18*

public:
	constexpr static height_size_type MAX_HEIGHT = sizeof(uint64_t) * 8;

	class iterator;

public:
	UnrolledSkipList(Allocator &allocator);
	UnrolledSkipList(UnrolledSkipList &&other);
	~UnrolledSkipList(){
		clear();
	}

	void print() const;

	void swap(UnrolledSkipList &other);

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
	template<bool ExactMatch>
	iterator find(std::string_view const key, std::bool_constant<ExactMatch>) const;
	iterator begin() const;
	static constexpr iterator end();

public:
	void printLanes() const;
	void printLane(height_size_type lane) const;
	void printLanesSummary() const;

private:
	struct 			Node;

	template<typename T>
	using HeightArray	= std::array<T, MAX_HEIGHT>;

	HeightArray<Node *>	heads_;

	ListCounter		lc_;

	Allocator		*allocator_;

private:
	void deallocate_(Node *node);

	void zeroing_();

	iterator fix_iterator_(const Node *node, typename MyPairVector::iterator           it) const;
	iterator fix_iterator_(const Node *node, typename MyPairVector::const_ptr_iterator it) const;

	struct NodeLocator;

	template<bool ShortcutEvaluation>
	NodeLocator locate_(std::string_view const key);

	static height_size_type getRandomHeight_();
};

// ==============================

template<class T_Allocator>
class UnrolledSkipList<T_Allocator>::iterator{
public:
	using MyPairVector		= UnrolledSkipList::MyPairVector;
	using MyPairVectorIterator	= typename MyPairVector::iterator;
	using MyPairVectorIteratorC	= typename MyPairVector::const_ptr_iterator;

	constexpr iterator(const Node *node) : node_(node){}
	constexpr iterator(const Node *node, MyPairVectorIterator it) : node_(node), it_(it){}
	explicit constexpr iterator(const Node *node, MyPairVectorIteratorC it) :
					iterator{
						node,
						MyPairVectorIterator{it}
					}{}

public:
	using difference_type = UnrolledSkipList::difference_type;
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
	const Node		*node_;
	MyPairVectorIterator	it_{};
};

// ==============================

template<class T_Allocator>
constexpr auto UnrolledSkipList<T_Allocator>::end() -> iterator{
	return { nullptr };
}

} // namespace

#endif

