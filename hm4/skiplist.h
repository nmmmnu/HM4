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

	class Iterator;

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
	Iterator lowerBound(StringRef const &key) const;
	Iterator begin() const;
	static constexpr Iterator end();

public:
	void printLanes() const;
	void printLane(height_size_type lane) const;

private:
	struct		Node;

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

class SkipList::Iterator{
private:
	friend class SkipList;
	constexpr Iterator(const Node *node) : node_(node){}

public:
	constexpr Iterator(Iterator const &other) = default;
	constexpr Iterator(Iterator &&other) = default;

public:
	using difference_type = SkipList::difference_type;
	using value_type = Pair;
	using pointer = const value_type *;
	using reference = value_type &;
	using iterator_category = std::input_iterator_tag;

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

inline auto SkipList::lowerBound(const StringRef &key) const -> Iterator{
	if (key.empty())
		return begin();

	return locateNode_(key, false);
}

inline auto SkipList::begin() const -> Iterator{
	return heads_[0];
}

constexpr auto SkipList::end() -> Iterator{
	return nullptr;
}


} // namespace

#endif
