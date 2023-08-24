#ifndef _SKIP_LIST_LIST_H
#define _SKIP_LIST_LIST_H

#include "ilist.h"
#include "listcounter.h"

#include <array>

namespace hm4{


template<class T_Allocator>
class SkipList{
public:
	using Allocator		= T_Allocator;

	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

	using height_size_type = uint8_t;

public:
	constexpr static height_size_type MAX_HEIGHT = sizeof(uint64_t) * 8;

	class iterator;

public:
	SkipList(Allocator &allocator);
	SkipList(SkipList &&other);
	~SkipList(){
		clear();
	}

	void swap(SkipList &other);

public:
	constexpr static std::string_view getName(){
		return "SkipList";
	}

public:
	bool clear();

	bool erase_(std::string_view const key);

	template<class PFactory>
	iterator insertF(PFactory &factory);

	auto size() const{
		return lc_.size();
	}

	auto const &mutable_list() const{
		return *this;
	}

	void mutable_notify(const Pair *, PairFactoryMutableNotifyMessage const &msg){
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
	template<bool B>
	iterator find(std::string_view const key, std::bool_constant<B> exact) const;
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

public:
	static size_t const INTERNAL_NODE_SIZE;

private:
	void deallocate_(Node *node);

	void zeroing_();

	struct NodeLocator;

	template<bool ShortcutEvaluation>
	NodeLocator locate_(std::string_view const key);

	static height_size_type getRandomHeight_();
};

// ==============================

template<class T_Allocator>
class SkipList<T_Allocator>::iterator{
public:
	constexpr iterator(const Node *node) : node_(node){}

public:
	using difference_type = SkipList::difference_type;
	using value_type = const Pair;
	using pointer = value_type *;
	using reference = value_type &;
	using iterator_category = std::forward_iterator_tag;

public:
	iterator &operator++();
	reference operator*() const;

public:
	bool operator==(const iterator &other) const{
		return node_ == other.node_;
	}

	bool operator!=(const iterator &other) const{
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
inline auto SkipList<T_Allocator>::begin() const -> iterator{
	return heads_[0];
}

template<class T_Allocator>
constexpr auto SkipList<T_Allocator>::end() -> iterator{
	return nullptr;
}

// ==============================

template<class T_Allocator>
inline auto swap(SkipList<T_Allocator> &a, SkipList<T_Allocator> &b){
	return a.swap(b);
}

} // namespace

#endif

