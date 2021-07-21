#include "skiplist.h"

#include <stdexcept>
#include <random>	// mt19937, bernoulli_distribution
#include <cassert>

#include "hpair.h"

namespace hm4{

namespace{
	std::mt19937_64 rand64{ (uint32_t) time(nullptr) };

	// unbelievably this is 4x faster
	// than DeBruijn-like algorithms,
	// e.g. 0x03F6EAF2CD271461

	template<typename T>
	constexpr T modifiedLSB(uint64_t x){
		T result = 1;

		while(x & 1u){
			++result;
			x = x >> 1;
		}

		return result;
	}
}

// ==============================

auto SkipList::getRandomHeight_() -> height_size_type{
	height_size_type const x = modifiedLSB<height_size_type>(rand64());

	// keeping this because of clang
	// don't optimize, it is branchless
	return std::clamp<height_size_type>(x, 1, MAX_HEIGHT);
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
	HPair::HKey	hkey;
	Pair		*data;
	Node		*next[1];	// system dependent, dynamic, at least 1

	static size_t calcSize(height_size_type const height){
		return sizeof(Node) + (height - 1) * sizeof(Node *);
	}

	int cmp(HPair::HKey const hkey, std::string_view const key) const{
		return HPair::cmp(this->hkey, *this->data, hkey, key);
	}
};

// ==============================

struct SkipList::NodeLocator{
	HeightArray<Node **>	prev;
	Node			*node	= nullptr;
};

// ==============================

SkipList::SkipList(Allocator &allocator) : allocator_(& allocator){
	zeroing_();
}

SkipList::SkipList(SkipList &&other):
		heads_		(std::move(other.heads_		)),
		lc_		(std::move(other.lc_		)),
		allocator_	(std::move(other.allocator_	)){
	other.zeroing_();
}

void SkipList::swap(SkipList &other){
	using std::swap;
	swap(heads_,		other.heads_		);
	swap(lc_,		other.lc_		);
	swap(allocator_,	other.allocator_	);
}

void SkipList::deallocate_(Node *node){
	allocator_->deallocate(node->data);
	allocator_->deallocate(node);
}

void SkipList::zeroing_(){
	lc_.clr();

	std::fill(heads_.begin(), heads_.end(), nullptr);
}

bool SkipList::clear(){
	if (allocator_->reset() == false){
		for(Node *node = heads_[0]; node; ){
			Node *copy = node;

			node = node->next[0];

			deallocate_(copy);
		}
	}

	zeroing_();

	return true;
}

template<class P>
auto SkipList::insert_(std::string_view key, P constructPair, size_t bytesPair) -> iterator{
	// create new node

	height_size_type const height = getRandomHeight_();

	size_t const bytesNode = Node::calcSize(height);

	std::byte *raw = static_cast<std::byte *>( allocator_->allocate(bytesNode + bytesPair) );

	if (raw == nullptr)
		return this->end();

	Node *newnode = reinterpret_cast<Node *>(raw + 0		);
	Pair *newdata = reinterpret_cast<Pair *>(raw + bytesPair	);

	constructPair(newdata);

	newnode->hkey = HPair::SS::create(key);
	newnode->data = newdata;

	const auto nl = locate_(key, std::false_type{});

	if (nl.node){
		const Pair *olddata = nl.node->data;

		// check if the data in database is valid
		if (! newdata->isValidForReplace(*olddata) ){
			allocator_->deallocate(newnode);

			return this->end();
		}

		// delete old node
		erase_(nl);
	}

	/* exchange pointers */
	{
		/* SEE REMARK ABOUT NEXT[] SIZE AT THE TOP */
		// newnode->height = height

		// place new node where it belongs
		for(height_size_type i = 0; i < height; ++i)
			newnode->next[i] = std::exchange(*nl.prev[i], newnode);

	#ifdef DEBUG_PRINT_LANES
		printf("%3u Lanes-> ", height);
		for(height_size_type i = 0; i < height; ++i)
			printf("%p ", (void *) newnode->next[i]);
		printf("\n");
	#endif

		/* SEE REMARK ABOUT NEXT[] SIZE AT THE TOP */
		// newnode->next[i] = NULL;
	}

	lc_.inc(bytesPair);

	return { newnode };
}

auto SkipList::insert(
		std::string_view key, std::string_view val,
		uint32_t expires, uint32_t created) -> iterator{

	if (! Pair::check(key, val))
		return this->end();

	return insert_(
		key,

		[&](Pair *mem){
			Pair::createInRawMemory(mem, key, val, expires, created);
		},

		Pair::bytes(key, val)
	);
}

auto SkipList::insert(Pair const &src) -> iterator{
	return insert_(
		src.getKey(),

		[&](Pair *mem){
			Pair::cloneInRawMemory(mem, src);
		},

		src.bytes()
	);
}


bool SkipList::erase_(NodeLocator const &nl){
	for(height_size_type h = 0; h < MAX_HEIGHT; ++h){
		if (*nl.prev[h] == nl.node)
			*nl.prev[h] = nl.node->next[h];
		else
			break;
	}

	lc_.dec( nl.node->data->bytes() );

	allocator_->deallocate(nl.node);

	return true;
}

bool SkipList::erase(std::string_view const key){
	// better Pair::check(key), but might fail because of the caller.
	assert(!key.empty());

	const auto nl = locate_(key, std::false_type{});

	if (nl.node == nullptr)
		return true;

	for(height_size_type h = 0; h < MAX_HEIGHT; ++h){
		if (*nl.prev[h] == nl.node)
			*nl.prev[h] = nl.node->next[h];
		else
			break;
	}

	lc_.dec( nl.node->data->bytes() );

	allocator_->deallocate(nl.node);

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
		hm4::print(*node->data);

		if (++i > 10)
			break;
	}
}

void SkipList::printLanesSummary() const{
	for(height_size_type lane = MAX_HEIGHT; lane --> 0;){
		uint64_t count = 0;
		for(const Node *node = heads_[lane]; node; node = node->next[lane])
			++count;

		printf("Lane # %5u -> %8zu\n", lane, count);
	}
}

// ==============================

template<bool ShortcutEvaluation>
auto SkipList::locate_(std::string_view const key, std::bool_constant<ShortcutEvaluation>) -> NodeLocator{
	if (key.empty()){
		// it is extremly dangerous to have key == nullptr here.
		throw std::logic_error{ "Key can not be nullptr in SkipList::locate_" };
	}

	NodeLocator nl;

	Node **jtable = heads_.data();

	auto hkey = HPair::SS::create(key);

	for(height_size_type h = MAX_HEIGHT; h --> 0;){
		for(Node *node = jtable[h]; node; node = node->next[h]){
			// this allows comparisson with single ">", instead of more complicated 3-way.
			if (node->hkey >= hkey){
				int const cmp = node->cmp(hkey, key);

				if (cmp >= 0){
					if (cmp == 0 && (h == 0 || ShortcutEvaluation)){
						// found
						nl.node = node;

						if constexpr(ShortcutEvaluation)
							return nl;
					}

					break;
				}

				// in rare corner case, it might go here.
			}

			jtable = node->next;
		}

		nl.prev[h] = & jtable[h];
	}

	return nl;
}

auto SkipList::locateNode_(std::string_view const key, bool const exact) const -> const Node *{
	if (key.empty()){
		// it is extremly dangerous to have key == nullptr here.
		throw std::logic_error{ "Key can not be nullptr in SkipList::locateNode_" };
	}

	const Node * const *jtable = heads_.data();

	const Node *node = nullptr;

	auto hkey = HPair::SS::create(key);

	for(height_size_type h = MAX_HEIGHT; h --> 0;){
		for(node = jtable[h]; node; node = node->next[h]){
			// this allows comparisson with single ">", instead of more complicated 3-way.
			if (node->hkey >= hkey){
				int const cmp = node->cmp(hkey, key);

				if (cmp >= 0){
					if (cmp == 0){
						// found
						return node;
					}

					break;
				}

				// in rare corner case, it might go here.
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

