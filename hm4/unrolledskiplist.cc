#include "unrolledskiplist.h"

#include <cassert>
#include <stdexcept>
#include <random>	// mt19937, bernoulli_distribution

#include "pmallocator.h"
#include "stdallocator.h"
#include "arenaallocator.h"
#include "simulatedarenaallocator.h"

#include "software_prefetch.h"

namespace hm4{

namespace{
	std::mt19937_64 rand64{ (uint32_t) time(nullptr) };

	// unbelievably this is 4x faster
	// than DeBruijn-like algorithms,
	// e.g. 0x03F6EAF2CD271461

	#if 0

	template<typename T>
	constexpr T clz(uint64_t x){
		T result = 0;

		while(x & 1u){
			++result;
			x = x >> 1;
		}

		return result;
	}

	#else

	template<typename T>
	constexpr T clz(uint64_t x){
		return static_cast<T>( __builtin_clzll(x) );
	}

	#endif
}

// ==============================

template<class T_Allocator>
auto UnrolledSkipList<T_Allocator>::getRandomHeight_() -> height_size_type{
	height_size_type const x = clz<height_size_type>(rand64()) + 1;

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
*/

constexpr bool DEBUG_PRINT_LANES = 0;

template<class T_Allocator>
struct UnrolledSkipList<T_Allocator>::Node{
	MyPairVector	data;
	Node		*next[1];	// system dependent, dynamic, at least 1

	constexpr static size_t calcSize(height_size_type const height){
		return sizeof(Node) + (height - 1) * sizeof(Node *);
	}

	int cmp(std::string_view const key) const{
		return data.back().cmp(key);
	}

	constexpr void prefetch(height_size_type const height) const{
		constexpr bool use_prefetch = true;

		if constexpr(use_prefetch){
			builtin_prefetch(& this->data.back(), 0, 1);
			builtin_prefetch(this->next[height], 0, 1);
		}
	}

	constexpr static auto begin_or_null(const Node *node){
		using It = typename MyPairVector::iterator;

		if (node)
			return node->data.begin();
		else
			return It{};
	}
};

template<class T_Allocator>
struct UnrolledSkipList<T_Allocator>::NodeLocator{
	HeightArray<Node **>	prev;
	Node			*node	= nullptr;
	bool			found	= false;

	void print(height_size_type max = MAX_HEIGHT) const{
		printf("Found: %d\n", (int   ) found	);
		printf("Node:  %p\n", (void *) node	);
		printf("Heights from: %u:\n", max);
		for(height_size_type h = max; h --> 0;)
			printf("%3u | %p\n", h, (void *)*prev[h]);
	}
};

namespace{
	// we not really need to check the integrity of the list
	constexpr bool corruptionCheck = false;

	[[maybe_unused]]
	void corruptionExit(){
		fprintf(stderr, "============================================\n");
		fprintf(stderr, "=== Detected UnrolledSkipList corruption ===\n");
		fprintf(stderr, "============================================\n");
		exit(100);
	}

}

// ==============================

template<class T_Allocator>
UnrolledSkipList<T_Allocator>::UnrolledSkipList(Allocator &allocator) : allocator_(& allocator){
	zeroing_();
}

template<class T_Allocator>
UnrolledSkipList<T_Allocator>::UnrolledSkipList(UnrolledSkipList &&other):
		heads_		(std::move(other.heads_		)),
		lc_		(std::move(other.lc_		)),
		allocator_	(std::move(other.allocator_	)){
	other.zeroing_();
}

template<class T_Allocator>
void UnrolledSkipList<T_Allocator>::swap(UnrolledSkipList &other){
	using std::swap;
	swap(heads_,		other.heads_		);
	swap(lc_,		other.lc_		);
	swap(allocator_,	other.allocator_	);
}

template<class T_Allocator>
void UnrolledSkipList<T_Allocator>::deallocate_(Node *node){
	using namespace MyAllocator;

	node->data.destruct(getAllocator());
	deallocate(allocator_, node);
}

template<class T_Allocator>
void UnrolledSkipList<T_Allocator>::zeroing_(){
	lc_.clr();

	std::fill(heads_.begin(), heads_.end(), nullptr);
}

template<class T_Allocator>
bool UnrolledSkipList<T_Allocator>::clear(){
	if (allocator_->reset() == false){
		for(Node *node = heads_[0]; node; ){
			node->prefetch(0);

			Node *copy = node;

			node = node->next[0];

			deallocate_(copy);
		}
	}

	zeroing_();

	return true;
}

template<class T_Allocator>
void UnrolledSkipList<T_Allocator>::print() const{
	printf("==begin list==\n");

	for(const Node *node = heads_[0]; node; node = node->next[0]){
		printf("Node: %p\n", (void *) node);

		printf("--begin data--\n");
		for(auto &x : node->data)
			x.print();

		printf("---end data---\n");
	}

	printf("===end list===\n\n\n\n\n");
}

template<class T_Allocator>
auto UnrolledSkipList<T_Allocator>::fix_iterator_(const Node *node, typename UnrolledSkipList::MyPairVector::iterator it) const -> iterator{
	if (it != node->data.end())
		return { node, it };

	if (!node->next[0])
		return end();

	node = node->next[0];
	return { node, node->data.begin() };
};

template<class T_Allocator>
auto UnrolledSkipList<T_Allocator>::fix_iterator_(const Node *node, typename UnrolledSkipList::MyPairVector::const_ptr_iterator it) const -> iterator{
	return fix_iterator_(node, typename UnrolledSkipList::MyPairVector::iterator(it));
}

template<class T_Allocator>
template<class PFactory>
auto UnrolledSkipList<T_Allocator>::insertF(PFactory &factory) -> iterator{
	auto constructNode = [](auto &allocator, height_size_type const height) -> Node *{
		using namespace MyAllocator;
		Node *newnode = allocate<Node>(
					allocator,
					Node::calcSize(height)
		);

		if (!newnode)
			return nullptr;

		newnode->data.construct();
		// next is not initialized
		// newnode->next = nullptr;

		return newnode;
	};

	auto connectNode = [](Node *newnode, NodeLocator const &nl, height_size_type const height){
		/* SEE REMARK ABOUT NEXT[] SIZE AT THE TOP */
		// newnode->height = height

		for(height_size_type i = 0; i < height; ++i)
			newnode->next[i] = std::exchange(*nl.prev[i], newnode);
	};

	auto const &key = factory.getKey();

	const auto nl = locate_<1>(key);

	if (nl.found){
		// update pair in place.

		auto const it = nl.node->data.insertF(factory, getAllocator(), lc_);

		return fix_iterator_(
			nl.node,
			it
		);
	}

	Node *node = nl.node;

	if (!node){
		// there is no node, make new one.

		// optimizing case when getRandomHeight_() == 1,
		// and proceeding like in LinkList,
		// DOES NOT PAY OFF AT ALL!
		height_size_type const height = getRandomHeight_();

		Node *newnode = constructNode(getAllocator(), height);

		if (!newnode)
			return end();

		auto const it = newnode->data.insertF(factory, getAllocator(), lc_);

		if (it == newnode->data.end()){
			// we can use smart_ptr here...
			deallocate_(newnode);
			return end();
		}

		connectNode(newnode, nl, height);

		return fix_iterator_(
			newnode,
			it
		);
	}

	if (node->data.full()){
		// current node is full, make new one and split elements.

		height_size_type const height = getRandomHeight_();

		Node *newnode = constructNode(getAllocator(), height);

		if (!newnode)
			return end();

		connectNode(newnode, nl, height);

		// node is connected in front.
		// must be in the back, but we can not do that.
		// so we do this instead:
		{
			using std::swap;
			swap(newnode, node);

			node->data.merge(newnode->data);
		}

		node->data.split(newnode->data);

		// is unclear where the pair should go
		// also we have knowledge how the node is split

		int const cmp = node->cmp(key);

		if (cmp >= 0){
			// insert in the old node

			auto const it = node->data.insertF(factory, getAllocator(), lc_);

			return fix_iterator_(
				node,
				it
			);
		}else{
			// insert in the new node

			auto const it = newnode->data.insertF(factory, getAllocator(), lc_);

			return fix_iterator_(
				newnode,
				it
			);
		}
	}

	// insert pair in current node.
	// TODO: optimize this, currently it do binary search over again.

	auto const it = node->data.insertF(factory, getAllocator(), lc_);

	return fix_iterator_(
		node,
		it
	);
}

template<class T_Allocator>
bool UnrolledSkipList<T_Allocator>::erase_(std::string_view const key){
	// better Pair::check(key), but might fail because of the caller.
	assert(!key.empty());

	const auto nl = locate_<0>(key);

	if (nl.node == nullptr)
		return false;

	// *nl.prev[0] is always valid.
	// it always point to the nl.node
	if constexpr(corruptionCheck)
		if (*nl.prev[0] != nl.node)
			corruptionExit();

	if (!nl.node->data.erase_(key, getAllocator(), lc_))
		return false;

	if (nl.node->data.size())
		return true;

	// node is zero size, it must be removed

	// unrolling the loop because in 50% of cases we have just this link...
	/* if constexpr unroll */ {
		height_size_type const h = 0;


		/* loop unroll */ {
			/* if unroll */ {
				*nl.prev[h] = nl.node->next[h];
			}
		}
	}

	// *nl.prev contains MAX_HEIGHT pointers.
	// some of them are nullptr, but they are present there.
	// some of them needs to be exchanged, some does not need.

	if constexpr(MAX_HEIGHT > 1){
		for(height_size_type h = 1; h < MAX_HEIGHT; ++h){
			if (*nl.prev[h] == nl.node)
				*nl.prev[h] = nl.node->next[h];
			else
				break;
		}
	}

	deallocate_(nl.node);

	return true;
}

// ==============================

template<class T_Allocator>
void UnrolledSkipList<T_Allocator>::printLanes() const{
	for(height_size_type lane = MAX_HEIGHT; lane --> 0;){
		printf("Lane # %5u :\n", lane);
		printLane(lane);
	}
}

template<class T_Allocator>
void UnrolledSkipList<T_Allocator>::printLane(height_size_type const lane) const{
	uint64_t i = 0;
	for(const Node *node = heads_[lane]; node; node = node->next[lane]){
		// no prefetch here.

		hm4::print(node->data.back());

		if (++i > 10)
			break;
	}
}

template<class T_Allocator>
void UnrolledSkipList<T_Allocator>::printLanesSummary() const{
	for(height_size_type lane = MAX_HEIGHT; lane --> 0;){
		uint64_t count = 0;
		for(const Node *node = heads_[lane]; node; node = node->next[lane]){
			// no prefetch here.

			++count;
		}

		printf("Lane # %5u -> %8zu\n", lane, count);
	}
}

// ==============================

template<class T_Allocator>
template<bool ShortcutEvaluation>
auto UnrolledSkipList<T_Allocator>::locate_(std::string_view const key) -> NodeLocator{
	if (key.empty()){
		// it is extremly dangerous to have key == nullptr here.
		throw std::logic_error{ "Key can not be nullptr in SkipList::locate_" };
	}

	NodeLocator nl;

	Node **jtable = heads_.data();

	static_assert(MAX_HEIGHT > 0);

	for(height_size_type h = MAX_HEIGHT; h --> 1;){
		for(Node *node = jtable[h]; node; node = node->next[h]){
			node->prefetch(h);

			int const cmp = node->cmp(key);

			if (cmp >= 0){
				nl.node = node;
				break;
			}

			// if is last node, drop down
			if (!node->next[h] && !node->data.full()){
				nl.node = node;
				break;
			}

			jtable = node->next;
		}

		nl.prev[h] = & jtable[h];
	}

	// level 0, like link list
	/* loop unroll */ {
		for(Node *node = jtable[0]; node; node = node->next[0]){
			node->prefetch(0);

			if (!node->next[0]){
				// last node
				nl.node = node;
				break;
			}

			int const cmp = node->cmp(key);

			if (cmp >= 0){
				// found
				nl.node  = node;
				nl.found = cmp == 0;
				break;
			}

			jtable = node->next;

		}

		nl.prev[0] = & jtable[0];
	}

	return nl;
}

template<class T_Allocator>
template<bool ExactMatch>
auto UnrolledSkipList<T_Allocator>::find(std::string_view const key, std::bool_constant<ExactMatch>) const -> iterator{
	if (key.empty()){
		// it is extremly dangerous to have key == nullptr here.
		throw std::logic_error{ "Key can not be nullptr in UnrolledSkipList::locateNode_" };
	}

	const Node * const *jtable = heads_.data();

	const Node *node = nullptr;
	int cmp = 0;

	static_assert(MAX_HEIGHT > 0);

	for(height_size_type h = MAX_HEIGHT; h --> 1;){
		for(node = jtable[h]; node; node = node->next[h]){
			node->prefetch(h);

			cmp = node->cmp(key);

			if (cmp >= 0){
				if (cmp == 0)
					goto done;
				else
					break;
			}

			jtable = node->next;
		}
	}

	// level 0, like link list
	/* loop unroll */ {
		for(node = jtable[0]; node; node = node->next[0]){
			node->prefetch(0);

			cmp = node->cmp(key);

			if (cmp >= 0)
				goto done;

			jtable = node->next;
		}
	}

	if (!node)
		return end();

	done:	// label for goto

	if (cmp == 0){
		// miracle, direct hit
		return { node, node->data.end() - 1 };
	}

	// search inside node

	auto const &[found, it] = node->data.locateC_(HPair::SS::create(key), key);

	if constexpr(ExactMatch)
		return found ? iterator{ node, it } : end();

	return fix_iterator_(node, it);
}


// ==============================


template<class T_Allocator>
auto UnrolledSkipList<T_Allocator>::iterator::operator++() -> iterator &{
	if (++it_ != node_->data.end())
		return *this;

	node_	= node_->next[0];
	it_	= Node::begin_or_null(node_);

	return *this;
}

template<class T_Allocator>
const Pair &UnrolledSkipList<T_Allocator>::iterator::operator*() const{
	return *it_;
}

template<class T_Allocator>
auto UnrolledSkipList<T_Allocator>::begin() const -> iterator{
	return iterator{ heads_[0], Node::begin_or_null(heads_[0]) };
}

// ==============================

template class UnrolledSkipList<MyAllocator::PMAllocator>;
template class UnrolledSkipList<MyAllocator::STDAllocator>;
template class UnrolledSkipList<MyAllocator::ArenaAllocator>;
template class UnrolledSkipList<MyAllocator::SimulatedArenaAllocator>;

template auto UnrolledSkipList<MyAllocator::PMAllocator>		::find(std::string_view const key, std::true_type ) const -> iterator;
template auto UnrolledSkipList<MyAllocator::STDAllocator>		::find(std::string_view const key, std::true_type ) const -> iterator;
template auto UnrolledSkipList<MyAllocator::ArenaAllocator>		::find(std::string_view const key, std::true_type ) const -> iterator;
template auto UnrolledSkipList<MyAllocator::SimulatedArenaAllocator>	::find(std::string_view const key, std::true_type ) const -> iterator;

template auto UnrolledSkipList<MyAllocator::PMAllocator>		::find(std::string_view const key, std::false_type) const -> iterator;
template auto UnrolledSkipList<MyAllocator::STDAllocator>		::find(std::string_view const key, std::false_type) const -> iterator;
template auto UnrolledSkipList<MyAllocator::ArenaAllocator>		::find(std::string_view const key, std::false_type) const -> iterator;
template auto UnrolledSkipList<MyAllocator::SimulatedArenaAllocator>	::find(std::string_view const key, std::false_type) const -> iterator;

template auto UnrolledSkipList<MyAllocator::PMAllocator>		::insertF(PairFactory::Normal		&factory) -> iterator;
template auto UnrolledSkipList<MyAllocator::STDAllocator>		::insertF(PairFactory::Normal		&factory) -> iterator;
template auto UnrolledSkipList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Normal		&factory) -> iterator;
template auto UnrolledSkipList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Normal		&factory) -> iterator;

template auto UnrolledSkipList<MyAllocator::PMAllocator>		::insertF(PairFactory::Expires		&factory) -> iterator;
template auto UnrolledSkipList<MyAllocator::STDAllocator>		::insertF(PairFactory::Expires		&factory) -> iterator;
template auto UnrolledSkipList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Expires		&factory) -> iterator;
template auto UnrolledSkipList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Expires		&factory) -> iterator;

template auto UnrolledSkipList<MyAllocator::PMAllocator>		::insertF(PairFactory::Tombstone	&factory) -> iterator;
template auto UnrolledSkipList<MyAllocator::STDAllocator>		::insertF(PairFactory::Tombstone	&factory) -> iterator;
template auto UnrolledSkipList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Tombstone	&factory) -> iterator;
template auto UnrolledSkipList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Tombstone	&factory) -> iterator;

template auto UnrolledSkipList<MyAllocator::PMAllocator>		::insertF(PairFactory::Clone		&factory) -> iterator;
template auto UnrolledSkipList<MyAllocator::STDAllocator>		::insertF(PairFactory::Clone		&factory) -> iterator;
template auto UnrolledSkipList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Clone		&factory) -> iterator;
template auto UnrolledSkipList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Clone		&factory) -> iterator;

template auto UnrolledSkipList<MyAllocator::PMAllocator>		::insertF(PairFactory::IFactory		&factory) -> iterator;
template auto UnrolledSkipList<MyAllocator::STDAllocator>		::insertF(PairFactory::IFactory		&factory) -> iterator;
template auto UnrolledSkipList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::IFactory		&factory) -> iterator;
template auto UnrolledSkipList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::IFactory		&factory) -> iterator;

} // namespace

