#ifndef _SKIP_LIST_LIST_H
#define _SKIP_LIST_LIST_H

#include "ilist.h"

#include <array>


namespace hm4{


class SkipList : public IList<SkipList, true>{
public:
	using height_type = uint8_t;

public:
	static constexpr height_type MAX_HEIGHT		= 64;
	static constexpr height_type DEFAULT_HEIGHT	= 32;

	class Iterator;

public:
	explicit
	SkipList(height_type height = DEFAULT_HEIGHT);
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
	using		NodeArray	= std::array<Node *, MAX_HEIGHT>;

	struct		NodeLocator{
				Node		*node	= nullptr;
				NodeArray	prev;
			};

	height_type	height_;
	NodeArray	heads_;

	size_type	dataCount_;
	size_t		dataSize_;

private:
	void clear_();

	NodeLocator locateMut_(const StringRef &key, bool complete_evaluation = false);
	const Node *locate_(const StringRef &key, bool const exact = true) const;

	height_type getRandomHeight_();

private:
	class RandomGenerator;

	static RandomGenerator rand_;
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

	const Node *node = locate_(key, false);
	return Iterator(node);
}

inline auto SkipList::begin() const -> Iterator{
	return Iterator(heads_[0]);
}

inline constexpr auto SkipList::end() -> Iterator{
	return Iterator(nullptr);
}


} // namespace

#endif
