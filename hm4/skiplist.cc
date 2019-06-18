#include "skiplist.h"

#include <stdexcept>
#include <algorithm>	// fill
#include <random>	// mt19937, bernoulli_distribution

namespace hm4{

namespace skiplist_impl_{
	std::mt19937_64 rand64{ (uint32_t) time(nullptr) };

	// unbelievably this is 4x faster
	// than DeBruijn-like algorithms,
	// e.g. 0x03F6EAF2CD271461

	template<typename T>
	constexpr T lsb(uint64_t const value, T const min, T const max){
		for(T i = min; i < max; ++i)
			if(value & (1u << i) )
				return i;

		return min;
	}
}

// ==============================

auto SkipList::getRandomHeight_() -> height_size_type{
	using namespace skiplist_impl_;

	uint64_t const rand = rand64();

	return lsb<height_size_type>(rand, 1, MAX_HEIGHT);
}

// ==============================

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
	// no need universal ref
	Node(OPair &&data) : data(std::move(data)){}

private:
	static size_t calcNewSize__(size_t const size, height_size_type const height){
		return size + (height - 1) * sizeof(Node *);
	}

public:
	static void *operator new(size_t const size, height_size_type const height) {
		return ::operator new(calcNewSize__(size, height));
	}

	static void *operator new(size_t const size, height_size_type const height, std::nothrow_t ) {
		return ::operator new(calcNewSize__(size, height), std::nothrow);
	}
};

// ==============================

struct SkipList::NodeLocator{
	HeightArray<Node **>	prev;
	Node			*node	= nullptr;
};

// ==============================

SkipList::SkipList(){
	zeroing_();
}

SkipList::SkipList(SkipList &&other):
		heads_		(std::move(other.heads_		)),
		dataCount_	(std::move(other.dataCount_	)),
		dataSize_	(std::move(other.dataSize_	)){
	other.zeroing_();
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

	zeroing_();

	return true;
}

bool SkipList::insert(OPair&& newdata){
	assert(newdata);

	const StringRef &key = newdata->getKey();

	const auto nl = locate_(key, true);

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
	height_size_type const height = getRandomHeight_();

	Node *newnode = new(height, std::nothrow) Node(std::move(newdata));

	if (newnode == nullptr){
		// newdata will be magically destroyed.
		return false;
	}

	/* SEE REMARK ABOUT NEXT[] SIZE AT THE TOP */
	// newnode->height = height

	// place new node where it belongs
	for(height_size_type i = 0; i < height; ++i){
		newnode->next[i] = *nl.prev[i];
		*nl.prev[i] = newnode;
	}

#ifdef DEBUG_PRINT_LANES
	printf("%3u Lanes-> ", height);
	for(height_size_type i = 0; i < height; ++i)
		printf("%p ", (void *) newnode->next[i]);
	printf("\n");
#endif

	/* SEE REMARK ABOUT NEXT[] SIZE AT THE TOP */
	// newnode->next[i] = NULL;

	dataSize_ += size;
	++dataCount_;

	return true;
}

#if 0
const Pair *SkipList::operator[](StringRef const &key) const{
	assert(!key.empty());

	const Node *node = locateNode_(key, true);

	return node ? node->data.get() : nullptr;
}
#endif

bool SkipList::erase(StringRef const &key){
	assert(!key.empty());

	const auto nl = locate_(key, false);

	if (nl.node == nullptr)
		return true;

	for(height_size_type h = 0; h < MAX_HEIGHT; ++h){
		if (*nl.prev[h] == nl.node)
			*nl.prev[h] = nl.node->next[h];
		else
			break;
	}

	const OPair & data = nl.node->data;

	dataSize_ -= data->bytes();
	dataCount_--;

	delete nl.node;

	return true;
}

// ==============================

void SkipList::printLanes() const{
	for(height_size_type lane = MAX_HEIGHT; lane --> 0;){
		printf("Lane # %5u :\n", lane);
		printLane(lane);
	}
}

void SkipList::printLane(height_size_type const lane) const{
	uint64_t i = 0;
	for(const Node *node = heads_[lane]; node; node = node->next[lane]){
		hm4::print(node->data);

		if (++i > 10)
			break;
	}
}

// ==============================

void SkipList::zeroing_(){
	dataSize_ = 0;
	dataCount_ = 0;

	std::fill(heads_.begin(), heads_.end(), nullptr);
}

auto SkipList::locate_(StringRef const &key, bool const shortcut_evaluation) -> NodeLocator{
	if (key.empty()){
		// it is extremly dangerous to have key == nullptr here.
		throw std::logic_error{ "Key can not be nullptr in SkipList::locate_" };
	}

	NodeLocator nl;

	Node **jtable = heads_.data();

	for(height_size_type h = MAX_HEIGHT; h --> 0;){
		for(Node *node = jtable[h]; node; node = node->next[h]){
			const OPair & data = node->data;
			int const cmp = data.cmp(key);

			if (cmp >= 0){
				if (cmp == 0 && (shortcut_evaluation || h == 0) ){
					// found
					nl.node = node;

					if (shortcut_evaluation){
						// at this point, we do not really care,
						// if nl.prev is setup correctly.
						return nl;
					}
				}

				break;
			}

			jtable = node->next;
		}

		nl.prev[h] = & jtable[h];
	}

	return nl;
}

auto SkipList::locateNode_(StringRef const &key, bool const exact) const -> const Node *{
	if (key.empty()){
		// it is extremly dangerous to have key == nullptr here.
		throw std::logic_error{ "Key can not be nullptr in SkipList::locateNode_" };
	}

	const Node * const *jtable = heads_.data();

	const Node *node = nullptr;

	for(height_size_type h = MAX_HEIGHT; h --> 0;){
		for(node = jtable[h]; node; node = node->next[h]){
			const OPair & data = node->data;
			int const cmp = data.cmp(key);

			if (cmp >= 0){
				if (cmp == 0){
					// found
					return node;
				}

				break;
			}

			jtable = node->next;
		}
	}

	return exact ? nullptr : node;
}


// ==============================


auto SkipList::iterator::operator++() -> iterator &{
	node_ = node_->next[0];
	return *this;
}

const Pair &SkipList::iterator::operator*() const{
	assert(node_);

	return *(node_->data);
}


} // namespace

