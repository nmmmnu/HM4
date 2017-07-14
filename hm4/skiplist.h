#ifndef _SKIP_LIST_LIST_H
#define _SKIP_LIST_LIST_H

#include "ilist.h"

#include <array>


namespace hm4{


class SkipList : public IMutableList<SkipList>{
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

	const Pair &operator[](const StringRef &key) const;
	bool erase(const StringRef &key);

	size_type size(bool const = true) const noexcept{
		return dataCount_;
	}

	size_t bytes() const noexcept{
		return dataSize_;
	}

public:
	Iterator lowerBound(const StringRef &key) const;
	Iterator begin() const;
	Iterator end() const;

private:
	friend class IMutableList<SkipList>;

	template <class UPAIR>
	bool insertT_(UPAIR &&data);

public:
	void printLanes() const;
	void printLane(height_type lane) const;

private:
	struct		Node;
	using		NodeArray	= std::array<      Node *, MAX_HEIGHT>;
	using		NodeArrayC	= std::array<const Node *, MAX_HEIGHT>;

	struct		NodeLocator{
				const Node *node;
				NodeArrayC loc;
			};

	height_type	height_;
	NodeArray	heads_;

	size_type	dataCount_;
	size_t		dataSize_;

private:
	void clear_();

	NodeLocator locate_(const StringRef &key, bool complete_evaluation = false) const;

	height_type _getRandomHeight();

private:
	class RandomGenerator;

	static RandomGenerator rand_;
};

// ==============================

class SkipList::Iterator{
private:
	friend class SkipList;
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

inline SkipList::Iterator SkipList::begin() const{
	return Iterator(heads_[0]);
}

inline SkipList::Iterator SkipList::end() const{
	return Iterator(nullptr);
}


} // namespace

#endif
