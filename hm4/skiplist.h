#ifndef _SKIP_LIST_LIST_H
#define _SKIP_LIST_LIST_H

#include "ilist.h"

#include <array>


namespace hm4{


class SkipList : public IList<SkipList, true>{
public:
	using height_type = uint8_t;

public:
	static constexpr height_type MAX_HEIGHT = sizeof(uint64_t) * 8;

	class Iterator;

public:
	SkipList();
	SkipList(SkipList &&other);
	~SkipList();

public:
	bool clear();

	const Pair *operator[](const StringRef &key) const;
	bool erase(const StringRef &key);

	bool insert(OPair &&data);

	size_type size(bool const = false) const noexcept{
		return dataCount_;
	}

	size_t bytes() const noexcept{
		return dataSize_;
	}

public:
	Iterator lowerBound(const StringRef &key) const;
	Iterator begin() const;
	static constexpr Iterator end();

public:
	void printLanes() const;
	void printLane(height_type lane) const;

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

	NodeLocator locate_(const StringRef &key, bool shortcut_evaluation);

	const Node *locateNode_(const StringRef &key, bool const exact) const;

	static height_type getRandomHeight_();
};

// ==============================

class SkipList::Iterator{
private:
	friend class SkipList;
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
