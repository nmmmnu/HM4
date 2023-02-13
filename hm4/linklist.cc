#include "linklist.h"

#include <cassert>

#include "hpair.h"

#include "pmallocator.h"
#include "stdallocator.h"
#include "arenaallocator.h"
#include "simulatedarenaallocator.h"

#include "software_prefetch.h"

namespace hm4{

template<class T_Allocator>
struct LinkList<T_Allocator>::Node{
	HPair::HKey	hkey;
	Pair		*data;
	Node		*next = nullptr;

	int cmp(HPair::HKey const hkey, std::string_view const key) const{
		return HPair::cmp(this->hkey, *this->data, hkey, key);
	}

	constexpr void prefetch() const{
		constexpr bool use_prefetch = true;

		if constexpr(use_prefetch){
			builtin_prefetch(this->data, 0, 1);
			builtin_prefetch(this->next, 0, 1);
		}
	}
};

template<class T_Allocator>
struct LinkList<T_Allocator>::NodeLocator{
	Node **prev;
	Node *node;
};

namespace{
	// we not really need to check the integrity of the list
	constexpr bool corruptionCheck = false;

	[[maybe_unused]]
	void corruptionExit(){
		fprintf(stderr, "====================================\n");
		fprintf(stderr, "=== Detected LinkList corruption ===\n");
		fprintf(stderr, "====================================\n");
		exit(100);
	}
}

// ==============================

template<class T_Allocator>
LinkList<T_Allocator>::LinkList(Allocator &allocator) : allocator_(& allocator){
	clear_();
}

template<class T_Allocator>
LinkList<T_Allocator>::LinkList(LinkList &&other):
			head_		(std::move(other.head_		)),
			lc_		(std::move(other.lc_		)),
			allocator_	(std::move(other.allocator_	)){
	other.clear_();
}

template<class T_Allocator>
void LinkList<T_Allocator>::deallocate_(Node *node){
	using namespace MyAllocator;

	deallocate(allocator_, node->data);
	deallocate(allocator_, node);
}

template<class T_Allocator>
void LinkList<T_Allocator>::clear_(){
	lc_.clr();

	head_ = nullptr;
}

template<class T_Allocator>
bool LinkList<T_Allocator>::clear(){
	if (allocator_->reset() == false){
		for(Node *node = head_; node; ){
			Node *copy = node;

			node->prefetch();

			node = node->next;

			deallocate_(copy);
		}
	}

	clear_();

	return true;
}

template<class T_Allocator>
template<class PFactory>
auto LinkList<T_Allocator>::insertLazyPair_(PFactory &&factory) -> iterator{
	auto const &key = factory.getKey();

	const auto loc = locate_(key);

	if (loc.node){
		// update node in place.

		Pair *olddata = loc.node->data;

		// try update pair in place.

		if (factory(olddata, *this)){
			// successfully updated.

			return { loc.node };
		}

		auto newdata = factory(getAllocator());

		if (!newdata)
			return this->end();

		if constexpr(config::LIST_CHECK_PAIR_FOR_REPLACE){
			// check if the data in database is valid
			if (! newdata->isValidForReplace(*olddata) ){
				// newdata will be magically destroyed.
				return this->end();
			}
		}

		lc_.upd( olddata->bytes(), newdata->bytes() );

		// assign new pair
		loc.node->hkey = HPair::SS::create(key);
		loc.node->data = newdata.release();

		// deallocate old pair
		using namespace MyAllocator;

		deallocate(allocator_, olddata);

		return { loc.node };
	}

	// create new node

	auto newdata = factory(getAllocator());

	if (!newdata)
		return this->end();

	size_t const size = newdata->bytes();

	using namespace MyAllocator;

	Node *newnode = allocate<Node>(allocator_);

	if (newnode == nullptr){
		// newdata will be magically destroyed.
		return this->end();
	}

	newnode->hkey = HPair::SS::create(key);
	newnode->data = newdata.release();

	newnode->next = std::exchange(*loc.prev, newnode);

	lc_.inc(size);

	return { newnode };
}

template<class T_Allocator>
bool LinkList<T_Allocator>::erase_(std::string_view const key){
	// better Pair::check(key), but might fail because of the caller.
	assert(!key.empty());

	auto loc = locate_(key);

	if (loc.node == nullptr)
		return false;

	if constexpr(corruptionCheck)
		if (*loc.prev != loc.node)
			corruptionExit();

	*loc.prev = loc.node->next;

	lc_.dec( loc.node->data->bytes() );

	deallocate_(loc.node);

	return true;
}

// ==============================

template<class T_Allocator>
auto LinkList<T_Allocator>::locate_(std::string_view const key) -> NodeLocator{
	// better Pair::check(key), but might fail because of the caller.
	assert(!key.empty());

	Node **jtable = & head_;

	auto hkey = HPair::SS::create(key);

	for(Node *node = *jtable; node; node = node->next){

		node->prefetch();

		// this allows comparisson with single ">", instead of more complicated 3-way.
		if (node->hkey >= hkey){
			int const cmp = node->cmp(hkey, key);

			if (cmp >= 0){
				if (cmp == 0)
					return { jtable, node };
				else
					return { jtable, nullptr };
			}

			// in rare corner case, it might go here.
		}

		jtable = & node->next;
	}

	return { jtable, nullptr };
}

template<class T_Allocator>
auto LinkList<T_Allocator>::locateNode_(std::string_view const key, bool const exact) const -> const Node *{
	assert(!key.empty());

	auto hkey = HPair::SS::create(key);

	for(const Node *node = head_; node; node = node->next){

		node->prefetch();

		// this allows comparisson with single ">", instead of more complicated 3-way.
		if (node->hkey >= hkey){
			int const cmp = node->cmp(hkey, key);

			if (cmp >= 0){
				if (cmp == 0)
					return node;
				else
					return exact ? nullptr : node;
			}

			// in rare corner case, it might go here.
		}
	}

	return nullptr;
}

// ==============================


template<class T_Allocator>
auto LinkList<T_Allocator>::iterator::operator++() -> iterator &{
	node_ = node_->next;

	return *this;
}

template<class T_Allocator>
const Pair &LinkList<T_Allocator>::iterator::operator*() const{
	assert(node_);

	return *(node_->data);
}

// ==============================

template class LinkList<MyAllocator::PMAllocator>;
template class LinkList<MyAllocator::STDAllocator>;
template class LinkList<MyAllocator::ArenaAllocator>;
template class LinkList<MyAllocator::SimulatedArenaAllocator>;

template auto LinkList<MyAllocator::PMAllocator>		::insertLazyPair_(PairFactory::Normal			&&factory) -> iterator;
template auto LinkList<MyAllocator::STDAllocator>		::insertLazyPair_(PairFactory::Normal			&&factory) -> iterator;
template auto LinkList<MyAllocator::ArenaAllocator>		::insertLazyPair_(PairFactory::Normal			&&factory) -> iterator;
template auto LinkList<MyAllocator::SimulatedArenaAllocator>	::insertLazyPair_(PairFactory::Normal			&&factory) -> iterator;

template auto LinkList<MyAllocator::PMAllocator>		::insertLazyPair_(PairFactory::NormalExpiresOnly	&&factory) -> iterator;
template auto LinkList<MyAllocator::STDAllocator>		::insertLazyPair_(PairFactory::NormalExpiresOnly	&&factory) -> iterator;
template auto LinkList<MyAllocator::ArenaAllocator>		::insertLazyPair_(PairFactory::NormalExpiresOnly	&&factory) -> iterator;
template auto LinkList<MyAllocator::SimulatedArenaAllocator>	::insertLazyPair_(PairFactory::NormalExpiresOnly	&&factory) -> iterator;

template auto LinkList<MyAllocator::PMAllocator>		::insertLazyPair_(PairFactory::Tombstone		&&factory) -> iterator;
template auto LinkList<MyAllocator::STDAllocator>		::insertLazyPair_(PairFactory::Tombstone		&&factory) -> iterator;
template auto LinkList<MyAllocator::ArenaAllocator>		::insertLazyPair_(PairFactory::Tombstone		&&factory) -> iterator;
template auto LinkList<MyAllocator::SimulatedArenaAllocator>	::insertLazyPair_(PairFactory::Tombstone		&&factory) -> iterator;

template auto LinkList<MyAllocator::PMAllocator>		::insertLazyPair_(PairFactory::Clone			&&factory) -> iterator;
template auto LinkList<MyAllocator::STDAllocator>		::insertLazyPair_(PairFactory::Clone			&&factory) -> iterator;
template auto LinkList<MyAllocator::ArenaAllocator>		::insertLazyPair_(PairFactory::Clone			&&factory) -> iterator;
template auto LinkList<MyAllocator::SimulatedArenaAllocator>	::insertLazyPair_(PairFactory::Clone			&&factory) -> iterator;

} // namespace


