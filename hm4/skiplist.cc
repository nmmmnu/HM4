#include "skiplist.h"

#include <stdexcept>
#include <algorithm>	// fill
#include <random>	// mt19937, bernoulli_distribution

namespace hm4{

class SkipList::RandomGenerator{
public:
	bool operator()(){
		return distr_(gen_);
	}

private:
	std::mt19937			gen_{ (uint32_t) time(nullptr) };
	std::bernoulli_distribution	distr_{ 0.5 };
};

SkipList::RandomGenerator SkipList::rand_;

/*
We do ***NOT*** store next[] array size,
***NOR*** we store NULL after last next node.

It turn out it does not need, because NULL lanes are already NULL.

Disadvantage is once allocated, no one knows the size,
except probably with malloc_usable_size();

[2]------------------------------->NULL
[1]------>[1]------>[1]----------->NULL
[0]->[0]->[0]->[0]->[0]->[0]->[0]->NULL

Uncommend DEBUG_PRINT_LANES for visualisation.

#define DEBUG_PRINT_LANES
*/

struct SkipList::Node{
	OPair	data;
	Node	*next[1];	// system dependent, dynamic, at least 1

public:
	Node(OPair &&data) : data(std::move(data)){}

public:
	static void *operator new(size_t size, height_type const height, bool const nothrow = false) {
		size += (height - 1) * sizeof(Node *);

		if (nothrow)
			return ::operator new(size, std::nothrow);
		else
			return ::operator new(size);
	}
};

SkipList::SkipList(height_type const height){
	if (height == 0 || height > MAX_HEIGHT)
		height_ = DEFAULT_HEIGHT;
	else
		height_ = height;

	clear_();
}

SkipList::SkipList(SkipList &&other):
		height_		(std::move(other.height_	)),
		heads_		(std::move(other.heads_		)),
		dataCount_	(std::move(other.dataCount_	)),
		dataSize_	(std::move(other.dataSize_	)){
	other.clear_();
}

SkipList::~SkipList(){
	clear();
}

bool SkipList::clear(){
	for(const Node *node = heads_[0]; node; ){
		const Node *copy = node;

		node = node->next[0];

		delete copy;
	}

	clear_();

	return true;
}

bool SkipList::insert(OPair&& newdata){
	assert(newdata);

	const StringRef &key = newdata->getKey();

	const auto nl = locateMutable_(key);

	if (nl.node){
		// update in place.

		OPair & olddata = nl.node->data;

		// check if the data in database is valid
		if (! newdata->isValid(*olddata) ){
			// newdata will be magically destroyed.
			return false;
		}

		dataSize_ = dataSize_
			- olddata->bytes()
			+ newdata->bytes();

		// copy assignment
		olddata = std::move(newdata);

		return true;
	}

	// create new node

	size_t const size = newdata->bytes();
	height_type const height = getRandomHeight_();

	Node *newnode = new(height, true) Node(std::move(newdata));

	if (newnode == nullptr){
		// newdata will be magically destroyed.
		return false;
	}

	/* SEE REMARK ABOUT NEXT[] SIZE AT THE TOP */
	// newnode->height = height

	// place new node where it belongs
	for(height_type i = 0; i < height; ++i){
		if (Node *prev = nl.prev[i]){
			// we are at the middle
			newnode->next[i] = prev->next[i];
			prev->next[i] = newnode;
		}else{
			newnode->next[i] = heads_[i];
			heads_[i] = newnode;
		}
	}

#ifdef DEBUG_PRINT_LANES
	printf("%3u Lanes-> ", height);
	for(height_type i = 0; i < height; ++i)
		printf("%p ", (void *) newnode->next[i]);
	printf("\n");
#endif

	/* SEE REMARK ABOUT NEXT[] SIZE AT THE TOP */
	// newnode->next[i] = NULL;

	dataSize_ += size;
	++dataCount_;

	return true;
}

const Pair *SkipList::operator[](const StringRef &key) const{
	assert(!key.empty());

	const Node *node = locate_(key);

	return node ? node->data.get() : nullptr;
}

bool SkipList::erase(const StringRef &key){
	assert(!key.empty());

	const auto nl = locateMutable_(key, true);

	if (nl.node == nullptr)
		return true;

	for(height_type i = 0; i < height_; ++i){
		if (Node *prev = nl.prev[i]){
			// check if lane go to this node...
			if (prev->next[i] == nl.node)
				prev->next[i] = nl.node->next[i];
		}else{
			// must be first
			if (heads_[i] == nl.node)
				heads_[i] = nl.node->next[i];
		}
	}

	const OPair & data = nl.node->data;

	dataSize_ -= data->bytes();
	dataCount_--;

	delete nl.node;

	return true;
}

// ==============================

void SkipList::printLanes() const{
	for(height_type i = height_; i > 0; --i){
		printf("Lane # %5u :\n", i - 1);
		printLane((height_type) (i - 1));
	}
}

void SkipList::printLane(height_type const lane) const{
	uint64_t i = 0;
	const Node *node;
	for(node = heads_[lane]; node; node = node->next[lane]){
		const OPair & data = node->data;
		print(data);

		if (++i > 10)
			break;
	}
}

// ==============================

void SkipList::clear_(){
	dataSize_ = 0;
	dataCount_ = 0;

	std::fill(heads_.begin(), heads_.end(), nullptr);
}

auto SkipList::locateMutable_(const StringRef &key, bool const complete_evaluation) -> NodeLocator{
	assert(!key.empty());

	if (key.empty()){
		// it is extremly dangerous to have key == nullptr here.
		throw std::logic_error{ "Key can not be nullptr in SkipList::locateMutable_" };
	}

	NodeLocator nl;

	// smart over-optimizations, such skip NULL lanes or
	// start from the middle of the list did not pay off.

	Node *prev = nullptr;

	height_type height = height_;
	while(height){
		Node *node = prev ? prev : heads_[height - 1];

		for(; node; node = node->next[height - 1]){
			const OPair & data = node->data;
			int const cmp = ocmp__(data, key);

			if (cmp == 0){
				// found

				// storing node here is redundant operation, however:
				// * cmp is const.
				// * one less branch
				nl.node =  node;

				if (complete_evaluation == false)
					return nl;

				break;
			}

			if (cmp > 0)
				break;

			prev = node;
		}

		nl.prev[height - 1] = prev;

		--height;
	}

	return nl;
}

auto SkipList::locate_(const StringRef &key, bool const exact) const -> const Node *{
	assert(!key.empty());

	if (key.empty()){
		// it is extremly dangerous to have key == nullptr here.
		throw std::logic_error{ "Key can not be nullptr in SkipList::locate_" };
	}

	const Node *node = nullptr;
	const Node *prev = nullptr;

	height_type height = height_;
	while(height){
		node = prev ? prev : heads_[height - 1];

		for(; node; node = node->next[height - 1]){
			const OPair & data = node->data;
			int const cmp = ocmp__(data, key);

			if (cmp == 0){
				// found
				return node;
			}

			if (cmp > 0)
				break;

			prev = node;
		}

		--height;
	}

	return exact ? nullptr : node;
}


auto SkipList::getRandomHeight_() -> height_type{
	// This gives slightly better performance,
	// than divide by 3 or multply by 0.33

	// We execute rand() inside the loop,
	// but performance is fast enought.

	height_type h = 1;
	while( h < height_ && rand_() )
		h++;

	return h;
}


// ==============================


SkipList::Iterator &SkipList::Iterator::operator++(){
	node_ = node_->next[0];
	return *this;
}

const Pair &SkipList::Iterator::operator*() const{
	assert(node_);

	return *(node_->data);
}


} // namespace

