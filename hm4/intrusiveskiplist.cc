#include "intrusiveskiplist.h"

#include <stdexcept>
#include <cstddef>
#include <random>	// mt19937, bernoulli_distribution
#include <cassert>

#include "hpair.h"

//#define DEBUG_PRINT_LANES 1

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

	auto getRandomHeight_(){
		using height_size_type = IntrusiveSkipList::height_size_type;

		height_size_type const x = modifiedLSB<height_size_type>(rand64());

		// keeping this because of clang
		// don't optimize, it is branchless
		return std::clamp<height_size_type>(x, 1, IntrusiveSkipList::MAX_HEIGHT);
	}
}

// ==============================

struct IntrusiveSkipList::Node{
	HPair::HKey		hkey;
	height_size_type	height;
	Node			*next[1];	// system dependent, dynamic, at least 1

	/* data goes here after last array element */

	[[nodiscard]]
	static size_t calcSize(height_size_type const height){
		return sizeof(Node) + (height - 1) * sizeof(Node *);
	}

	[[nodiscard]]
	size_t calcSize() const{
		return calcSize(height);
	}

	[[nodiscard]]
	auto &getPair() const{
		using TByte = const std::byte;
		using TPair = const Pair;

		return *reinterpret_cast<TPair *>(
				reinterpret_cast<TByte *>(this) + calcSize()
		);
	}

	[[nodiscard]]
	auto &getPair(){
		using TByte = std::byte;
		using TPair = Pair;

		return *reinterpret_cast<TPair *>(
				reinterpret_cast<TByte *>(this) + calcSize()
		);
	}

	[[nodiscard]]
	int cmp(HPair::HKey const hkey, std::string_view const key) const{
		return HPair::cmp(this->hkey, getPair(), hkey, key);
	}
};




// ==============================

struct IntrusiveSkipList::NodeLocator{
	HeightArray<Node **>	prev;
	Node			*node	= nullptr;
};

// ==============================

IntrusiveSkipList::IntrusiveSkipList(Allocator &allocator) : allocator_(& allocator){
	static_assert(std::is_trivial_v<IntrusiveSkipList::Node>, "IntrusiveSkipList::Node must be POD type");

	static_assert(sizeof(IntrusiveSkipList::Node) == 24, "sizeof not ok");

	zeroing_();
}

IntrusiveSkipList::IntrusiveSkipList(IntrusiveSkipList &&other):
		heads_		(std::move(other.heads_		)),
		lc_		(std::move(other.lc_		)),
		allocator_	(std::move(other.allocator_	)){
	other.zeroing_();
}

/*
void IntrusiveSkipList::swap(IntrusiveSkipList &other){
	using std::swap;
	swap(heads_,		other.heads_		);
	swap(lc_,		other.lc_		);
	swap(allocator_,	other.allocator_	);
	swap(allocator_,	other.allocator_	);
}
*/

void IntrusiveSkipList::zeroing_(){
	lc_.clr();

	std::fill(heads_.begin(), heads_.end(), nullptr);
}

bool IntrusiveSkipList::clear(){
	if (allocator_->reset() == false){
		for(Node *node = heads_[0]; node; ){
			Node *copy = node;

			node = node->next[0];

			allocator_->deallocate(copy);
		}
	}

	zeroing_();

	return true;
}

template<class Bytes, class Create>
auto IntrusiveSkipList::insert_(std::string_view const key, Bytes bytes, Create create) -> iterator{
	height_size_type const height = getRandomHeight_();

	size_t const size = bytes();

	size_t const nodeSize = Node::calcSize(height) + size;

	Node *newnode = reinterpret_cast<Node *>( allocator_->allocate(nodeSize) );

	if (newnode == nullptr)
		return this->end();

	newnode->hkey   = HPair::SS::create(key);
	newnode->height = height;

	create(& newnode->getPair());

	const auto nl = locate_(key, false);

	if (nl.node){
		// check if the data in database is valid
		if (! newnode->getPair().isValidForReplace(nl.node->getPair()) ){
			allocator_->deallocate(newnode);
			return this->end();
		}

		eraseNode_(nl);

		// fallthrought
	}

	/* exchange pointers */
	{
		// place new node where it belongs
		for(height_size_type i = 0; i < height; ++i)
			newnode->next[i] = std::exchange(*nl.prev[i], newnode);

	#ifdef DEBUG_PRINT_LANES
		printf("%3u Lanes-> ", height);
		for(height_size_type i = 0; i < height; ++i)
			printf("%p ", (void *) newnode->next[i]);
		printf("\n");
	#endif
	}

	lc_.inc(size);

	return { newnode };
}

auto IntrusiveSkipList::insert(	std::string_view const key, std::string_view const val,
				uint32_t const expires, uint32_t const created) -> iterator{

	if (Pair::check(key, val) == false)
		return this->end();

	return insert_(
		key,
		[&](){
			return Pair::bytes(key, val);
		},
		[&](Pair *mem){
			Pair::createInRawMemory(mem, key, val, expires, created);
		}
	);
}

auto IntrusiveSkipList::insert(Pair const &src) -> iterator{
	return insert_(
		src.getKey(),
		[&](){
			return src.bytes();
		},
		[&](Pair *mem){
			Pair::cloneInRawMemory(mem, src);
		}
	);
}

void IntrusiveSkipList::eraseNode_(NodeLocator const &nl){
	for(height_size_type h = 0; h < MAX_HEIGHT; ++h){
		if (*nl.prev[h] == nl.node)
			*nl.prev[h] = nl.node->next[h];
		else
			break;
	}

	lc_.dec( nl.node->getPair().bytes() );

	allocator_->deallocate(nl.node);
}

bool IntrusiveSkipList::erase(std::string_view const key){
	// better Pair::check(key), but might fail because of the caller.
	assert(!key.empty());

	const auto nl = locate_(key, false);

	if (nl.node){
		eraseNode_(nl);
		return true;
	}

	return false;
}

// ==============================

void IntrusiveSkipList::printLanes() const{
	for(height_size_type lane = MAX_HEIGHT; lane --> 0;){
		printf("Lane # %5u :\n", lane);
		printLane(lane);
	}
}

void IntrusiveSkipList::printLane(height_size_type const lane) const{
	uint64_t i = 0;
	for(const Node *node = heads_[lane]; node; node = node->next[lane]){
		hm4::print(node->getPair());

		if (++i > 10)
			break;
	}
}

// ==============================

auto IntrusiveSkipList::locate_(std::string_view const key, bool const shortcut_evaluation) -> NodeLocator{
	if (key.empty()){
		// it is extremly dangerous to have key == nullptr here.
		throw std::logic_error{ "Key can not be nullptr in IntrusiveSkipList::locate_" };
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

				// in rare corner case, it might go here.
			}

			jtable = node->next;
		}

		nl.prev[h] = & jtable[h];
	}

	return nl;
}

auto IntrusiveSkipList::locateNode_(std::string_view const key, bool const exact) const -> const Node *{
	if (key.empty()){
		// it is extremly dangerous to have key == nullptr here.
		throw std::logic_error{ "Key can not be nullptr in IntrusiveSkipList::locateNode_" };
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


auto IntrusiveSkipList::iterator::operator++() -> iterator &{
	node_ = node_->next[0];

	return *this;
}

const Pair &IntrusiveSkipList::iterator::operator*() const{
	assert(node_);

	return node_->getPair();
}


} // namespace

