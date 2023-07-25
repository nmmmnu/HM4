#include "skiplist.h"

#include <stdexcept>
#include <random>	// mt19937, bernoulli_distribution
#include <cassert>

#include "hpair.h"

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
	constexpr T CLZ(uint64_t x){
		T result = 1;

		while(x & 1u){
			++result;
			x = x >> 1;
		}

		return result;
	}

	#else

	template<typename T>
	constexpr T CLZ(uint64_t x){
		return static_cast<T>(__builtin_clzll(x) + 1);
	}

	#endif



	// we not really need to check the integrity of the list
	constexpr bool corruptionCheck = false;

	[[maybe_unused]]
	void corruptionExit(){
		fprintf(stderr, "====================================\n");
		fprintf(stderr, "=== Detected SkipList corruption ===\n");
		fprintf(stderr, "====================================\n");
		exit(100);
	}
}

// ==============================

template<class T_Allocator>
auto SkipList<T_Allocator>::getRandomHeight_() -> height_size_type{
	height_size_type const x = CLZ<height_size_type>(rand64());

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

template<class T_Allocator>
struct SkipList<T_Allocator>::Node{
	HPair::HKey	hkey;
	Pair		*data;
	Node		*next[1];	// system dependent, dynamic, at least 1

	constexpr static size_t calcSize(height_size_type const height){
		return sizeof(Node) + (height - 1) * sizeof(Node *);
	}

	int cmp(HPair::HKey const hkey, std::string_view const key) const{
		return HPair::cmp(this->hkey, *this->data, hkey, key);
	}

	constexpr void prefetch(height_size_type const height) const{
		constexpr bool use_prefetch = true;

		if constexpr(use_prefetch){
			builtin_prefetch(this->data,         0, 1);
			builtin_prefetch(this->next[height], 0, 1);
		}
	}
};

// ==============================

template<class T_Allocator>
struct SkipList<T_Allocator>::NodeLocator{
	HeightArray<Node **>	prev;
	Node			*node	= nullptr;
};

// ==============================

template<class T_Allocator>
SkipList<T_Allocator>::SkipList(Allocator &allocator) : allocator_(& allocator){
	zeroing_();
}

template<class T_Allocator>
SkipList<T_Allocator>::SkipList(SkipList &&other):
		heads_		(std::move(other.heads_		)),
		lc_		(std::move(other.lc_		)),
		allocator_	(std::move(other.allocator_	)){
	other.zeroing_();
}

template<class T_Allocator>
void SkipList<T_Allocator>::swap(SkipList &other){
	using std::swap;
	swap(heads_,		other.heads_		);
	swap(lc_,		other.lc_		);
	swap(allocator_,	other.allocator_	);
}

template<class T_Allocator>
void SkipList<T_Allocator>::deallocate_(Node *node){
	using namespace MyAllocator;

	deallocate(allocator_, node->data);
	deallocate(allocator_, node);
}

template<class T_Allocator>
void SkipList<T_Allocator>::zeroing_(){
	lc_.clr();

	std::fill(heads_.begin(), heads_.end(), nullptr);
}

template<class T_Allocator>
bool SkipList<T_Allocator>::clear(){
	if (allocator_->reset() == false){
		for(Node *node = heads_[0]; node; ){
			Node *copy = node;

			node->prefetch(0);

			node = node->next[0];

			deallocate_(copy);
		}
	}

	zeroing_();

	return true;
}

template<class T_Allocator>
template<class PFactory>
auto SkipList<T_Allocator>::insertF(PFactory &factory) -> iterator{
	auto const &key = factory.getKey();

	const auto nl = locate_(key, std::true_type{});

	if (nl.node){
		// update in place.

		Pair *olddata = nl.node->data;

		// check if we can update

		if constexpr(config::LIST_CHECK_PAIR_FOR_REPLACE){
			if (!isValidForReplace(factory.getCreated(), *olddata))
				return this->end();
		}

		// try update pair in place.
		if (auto const old_bytes = olddata->bytes(); tryInsertHint_(*this, olddata, factory)){
			// successfully updated.

			lc_.upd(old_bytes, factory.bytes());

			return { nl.node };
		}

		auto newdata = Pair::smart_ptr::create(getAllocator(), factory);

		if (!newdata)
			return this->end();

		lc_.upd( olddata->bytes(), newdata->bytes() );

		// assign new pair
		nl.node->hkey = HPair::SS::create(key);
		nl.node->data = newdata.release();

		using namespace MyAllocator;

		// deallocate old pair
		deallocate(allocator_, olddata);

		return { nl.node };
	}

	// create new node

	auto newdata = Pair::smart_ptr::create(getAllocator(), factory);

	if (!newdata)
		return this->end();

	size_t const size = newdata->bytes();

	height_size_type const height = getRandomHeight_();

	using namespace MyAllocator;

	Node *newnode = allocate<Node>(
				allocator_,
				Node::calcSize(height)
	);

	if (newnode == nullptr){
		// newdata will be magically destroyed.
		return this->end();
	}

	newnode->hkey = HPair::SS::create(key);
	newnode->data = newdata.release();

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

	lc_.inc(size);

	return { newnode };
}

template<class T_Allocator>
bool SkipList<T_Allocator>::erase_(std::string_view const key){
	// better Pair::check(key), but might fail because of the caller.
	assert(!key.empty());

	const auto nl = locate_(key, std::false_type{});

	if (nl.node == nullptr)
		return false;


	// *nl.prev[0] is always valid.
	// it always point to the nl.node
	// unrolling...

	/* if constexpr unroll */ {
		height_size_type const h = 0;

		if constexpr(corruptionCheck)
			if (*nl.prev[h] != nl.node)
				corruptionExit();

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

	lc_.dec( nl.node->data->bytes() );

	deallocate_(nl.node);

	return true;
}

// ==============================

template<class T_Allocator>
void SkipList<T_Allocator>::printLanes() const{
	for(height_size_type lane = MAX_HEIGHT; lane --> 0;){
		printf("Lane # %5u :\n", lane);
		printLane(lane);
	}
}

template<class T_Allocator>
void SkipList<T_Allocator>::printLane(height_size_type const lane) const{
	uint64_t i = 0;
	for(const Node *node = heads_[lane]; node; node = node->next[lane]){
		// no prefetch here.

		hm4::print(*node->data);

		if (++i > 10)
			break;
	}
}

template<class T_Allocator>
void SkipList<T_Allocator>::printLanesSummary() const{
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
auto SkipList<T_Allocator>::locate_(std::string_view const key, std::bool_constant<ShortcutEvaluation>) -> NodeLocator{
	if (key.empty()){
		// it is extremly dangerous to have key == nullptr here.
		throw std::logic_error{ "Key can not be nullptr in SkipList::locate_" };
	}

	NodeLocator nl;

	Node **jtable = heads_.data();

	auto hkey = HPair::SS::create(key);

	for(height_size_type h = MAX_HEIGHT; h --> 0;){
		for(Node *node = jtable[h]; node; node = node->next[h]){

			node->prefetch(h);

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

template<class T_Allocator>
template<bool ExactEvaluation>
auto SkipList<T_Allocator>::locateNode_(std::string_view const key, std::bool_constant<ExactEvaluation>) const -> const Node *{
	if (key.empty()){
		// it is extremly dangerous to have key == nullptr here.
		throw std::logic_error{ "Key can not be nullptr in SkipList::locateNode_" };
	}

	const Node * const *jtable = heads_.data();

	const Node *node = nullptr;

	auto hkey = HPair::SS::create(key);

	for(height_size_type h = MAX_HEIGHT; h --> 0;){
		for(node = jtable[h]; node; node = node->next[h]){

			node->prefetch(h);

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

	if constexpr(ExactEvaluation)
		return nullptr;
	else
		return node;
}


// ==============================


template<class T_Allocator>
auto SkipList<T_Allocator>::iterator::operator++() -> iterator &{
	node_ = node_->next[0];

	return *this;
}

template<class T_Allocator>
const Pair &SkipList<T_Allocator>::iterator::operator*() const{
	assert(node_);

	return *(node_->data);
}

// ==============================

template class SkipList<MyAllocator::PMAllocator>;
template class SkipList<MyAllocator::STDAllocator>;
template class SkipList<MyAllocator::ArenaAllocator>;
template class SkipList<MyAllocator::SimulatedArenaAllocator>;

template auto SkipList<MyAllocator::PMAllocator>		::locateNode_(std::string_view const key, std::true_type ) const -> const Node *;
template auto SkipList<MyAllocator::STDAllocator>		::locateNode_(std::string_view const key, std::true_type ) const -> const Node *;
template auto SkipList<MyAllocator::ArenaAllocator>		::locateNode_(std::string_view const key, std::true_type ) const -> const Node *;
template auto SkipList<MyAllocator::SimulatedArenaAllocator>	::locateNode_(std::string_view const key, std::true_type ) const -> const Node *;

template auto SkipList<MyAllocator::PMAllocator>		::locateNode_(std::string_view const key, std::false_type) const -> const Node *;
template auto SkipList<MyAllocator::STDAllocator>		::locateNode_(std::string_view const key, std::false_type) const -> const Node *;
template auto SkipList<MyAllocator::ArenaAllocator>		::locateNode_(std::string_view const key, std::false_type) const -> const Node *;
template auto SkipList<MyAllocator::SimulatedArenaAllocator>	::locateNode_(std::string_view const key, std::false_type) const -> const Node *;

template auto SkipList<MyAllocator::PMAllocator>		::insertF(PairFactory::Normal		&factory) -> iterator;
template auto SkipList<MyAllocator::STDAllocator>		::insertF(PairFactory::Normal		&factory) -> iterator;
template auto SkipList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Normal		&factory) -> iterator;
template auto SkipList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Normal		&factory) -> iterator;

template auto SkipList<MyAllocator::PMAllocator>		::insertF(PairFactory::Expires		&factory) -> iterator;
template auto SkipList<MyAllocator::STDAllocator>		::insertF(PairFactory::Expires		&factory) -> iterator;
template auto SkipList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Expires		&factory) -> iterator;
template auto SkipList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Expires		&factory) -> iterator;

template auto SkipList<MyAllocator::PMAllocator>		::insertF(PairFactory::Tombstone	&factory) -> iterator;
template auto SkipList<MyAllocator::STDAllocator>		::insertF(PairFactory::Tombstone	&factory) -> iterator;
template auto SkipList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Tombstone	&factory) -> iterator;
template auto SkipList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Tombstone	&factory) -> iterator;

template auto SkipList<MyAllocator::PMAllocator>		::insertF(PairFactory::Clone		&factory) -> iterator;
template auto SkipList<MyAllocator::STDAllocator>		::insertF(PairFactory::Clone		&factory) -> iterator;
template auto SkipList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Clone		&factory) -> iterator;
template auto SkipList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Clone		&factory) -> iterator;

template auto SkipList<MyAllocator::PMAllocator>		::insertF(PairFactory::IFactory		&factory) -> iterator;
template auto SkipList<MyAllocator::STDAllocator>		::insertF(PairFactory::IFactory		&factory) -> iterator;
template auto SkipList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::IFactory		&factory) -> iterator;
template auto SkipList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::IFactory		&factory) -> iterator;

} // namespace

