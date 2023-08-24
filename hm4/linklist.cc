#include "linklist.h"

#include "hpair.h"

#include "ilist_updateinplace.h"

#include <cassert>

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

template<class T_Allocator>
size_t const LinkList<T_Allocator>::INTERNAL_NODE_SIZE = sizeof(Node);

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
auto LinkList<T_Allocator>::insertF(PFactory &factory) -> iterator{
	auto const &key = factory.getKey();

	const auto nl = locate_(key);

	if (nl.node){
		// update node in place.

		Pair *olddata = nl.node->data;

		// check if we can update

		if constexpr(config::LIST_CHECK_PAIR_FOR_REPLACE)
			if (!isValidForReplace(factory.getCreated(), *olddata))
				return end();

		// try update pair in place.
		if (tryUpdateInPlaceLC(getAllocator(), olddata, factory, lc_)){
			// successfully updated.
			return { nl.node };
		}

		auto newdata = Pair::smart_ptr::create(getAllocator(), factory);

		if (!newdata)
			return this->end();

		lc_.upd( olddata->bytes(), newdata->bytes() );

		// assign new pair
		nl.node->hkey = HPair::SS::create(key);
		nl.node->data = newdata.release();

		// deallocate old pair
		using namespace MyAllocator;
		deallocate(allocator_, olddata);

		return { nl.node };
	}

	// create new node

	auto newdata = Pair::smart_ptr::create(getAllocator(), factory);

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

	newnode->next = std::exchange(*nl.prev, newnode);

	lc_.inc(size);

	return { newnode };
}

template<class T_Allocator>
bool LinkList<T_Allocator>::erase_(std::string_view const key){
	// better Pair::check(key), but might fail because of the caller.
	assert(!key.empty());

	auto nl = locate_(key);

	if (nl.node == nullptr)
		return false;

	if constexpr(corruptionCheck)
		if (*nl.prev != nl.node)
			corruptionExit();

	*nl.prev = nl.node->next;

	lc_.dec( nl.node->data->bytes() );

	deallocate_(nl.node);

	return true;
}

// ==============================

template<class T_Allocator>
auto LinkList<T_Allocator>::locate_(std::string_view const key) -> NodeLocator{
	// better Pair::check(key), but might fail because of the caller.
	assert(!key.empty());

	Node **jtable = & head_;

	auto const hkey = HPair::SS::create(key);

	for(Node *node = *jtable; node; node = node->next){
		node->prefetch();

		// this allows comparisson with single ">", instead of more complicated 3-way.
		if (node->hkey >= hkey){
			if (int const cmp = node->cmp(hkey, key); cmp >= 0){
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
template<bool ExactEvaluation>
auto LinkList<T_Allocator>::find(std::string_view const key, std::bool_constant<ExactEvaluation>) const -> iterator{
	assert(!key.empty());

	auto const hkey = HPair::SS::create(key);

	const Node *node;

	for(node = head_; node; node = node->next){
		node->prefetch();

		// this allows comparisson with single ">", instead of more complicated 3-way.
		if (node->hkey >= hkey){
			if (int const cmp = node->cmp(hkey, key); cmp >= 0){
				if (cmp == 0){
					// found
					return node;
				}

				break;
			}
		}
	}

	if constexpr(ExactEvaluation)
		return nullptr;
	else
		return node;
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

template auto LinkList<MyAllocator::PMAllocator>		::find(std::string_view const key, std::true_type ) const -> iterator;
template auto LinkList<MyAllocator::STDAllocator>		::find(std::string_view const key, std::true_type ) const -> iterator;
template auto LinkList<MyAllocator::ArenaAllocator>		::find(std::string_view const key, std::true_type ) const -> iterator;
template auto LinkList<MyAllocator::SimulatedArenaAllocator>	::find(std::string_view const key, std::true_type ) const -> iterator;

template auto LinkList<MyAllocator::PMAllocator>		::find(std::string_view const key, std::false_type) const -> iterator;
template auto LinkList<MyAllocator::STDAllocator>		::find(std::string_view const key, std::false_type) const -> iterator;
template auto LinkList<MyAllocator::ArenaAllocator>		::find(std::string_view const key, std::false_type) const -> iterator;
template auto LinkList<MyAllocator::SimulatedArenaAllocator>	::find(std::string_view const key, std::false_type) const -> iterator;

template auto LinkList<MyAllocator::PMAllocator>		::insertF(PairFactory::Normal		&factory) -> iterator;
template auto LinkList<MyAllocator::STDAllocator>		::insertF(PairFactory::Normal		&factory) -> iterator;
template auto LinkList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Normal		&factory) -> iterator;
template auto LinkList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Normal		&factory) -> iterator;

template auto LinkList<MyAllocator::PMAllocator>		::insertF(PairFactory::Expires		&factory) -> iterator;
template auto LinkList<MyAllocator::STDAllocator>		::insertF(PairFactory::Expires		&factory) -> iterator;
template auto LinkList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Expires		&factory) -> iterator;
template auto LinkList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Expires		&factory) -> iterator;

template auto LinkList<MyAllocator::PMAllocator>		::insertF(PairFactory::Tombstone	&factory) -> iterator;
template auto LinkList<MyAllocator::STDAllocator>		::insertF(PairFactory::Tombstone	&factory) -> iterator;
template auto LinkList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Tombstone	&factory) -> iterator;
template auto LinkList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Tombstone	&factory) -> iterator;

template auto LinkList<MyAllocator::PMAllocator>		::insertF(PairFactory::Clone		&factory) -> iterator;
template auto LinkList<MyAllocator::STDAllocator>		::insertF(PairFactory::Clone		&factory) -> iterator;
template auto LinkList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Clone		&factory) -> iterator;
template auto LinkList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Clone		&factory) -> iterator;

template auto LinkList<MyAllocator::PMAllocator>		::insertF(PairFactory::IFactory		&factory) -> iterator;
template auto LinkList<MyAllocator::STDAllocator>		::insertF(PairFactory::IFactory		&factory) -> iterator;
template auto LinkList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::IFactory		&factory) -> iterator;
template auto LinkList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::IFactory		&factory) -> iterator;

} // namespace


