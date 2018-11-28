#ifndef _SKIP_LIST_LIST_H
#define _SKIP_LIST_LIST_H

#include "ilist.h"

#include <array>


namespace hm4{


class SkipList{
public:
	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

	using height_size_type = uint8_t;

public:
	constexpr static height_size_type MAX_HEIGHT = sizeof(uint64_t) * 8;

	class iterator;

public:
	SkipList();
	SkipList(SkipList &&other);
	~SkipList();

public:
	bool clear();

	const Pair *operator[](StringRef const &key) const;
	bool erase(StringRef const &key);

	bool insert(OPair &&data);

	size_type size() const noexcept{
		return dataCount_;
	}

	size_t bytes() const noexcept{
		return dataSize_;
	}

public:
	iterator lowerBound(StringRef const &key) const;
	iterator begin() const;
	static constexpr iterator end();

public:
	void printLanes() const;
	void printLane(height_size_type lane) const;

private:
	struct 			Node;

	template<typename T>
	using HeightArray	= std::array<T,  MAX_HEIGHT>;

	HeightArray<Node *>	heads_;

	size_type		dataCount_;
	size_t			dataSize_;

private:
	void zeroing_();

	struct NodeLocator;

	NodeLocator locate_(StringRef const &key, bool shortcut_evaluation);

	const Node *locateNode_(StringRef const &key, bool const exact) const;

	static height_size_type getRandomHeight_();
};

// ==============================

class SkipList::iterator{
private:
	friend class SkipList;
	constexpr iterator(const Node *node) : node_(node){}

public:
	constexpr iterator(iterator const &other) = default;
	constexpr iterator(iterator &&other) = default;

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

inline auto SkipList::lowerBound(const StringRef &key) const -> iterator{
	if (key.empty())
		return begin();

	return locateNode_(key, false);
}

inline auto SkipList::begin() const -> iterator{
	return heads_[0];
}

constexpr auto SkipList::end() -> iterator{
	return nullptr;
}


} // namespace

#endif
