#include "unrolledlinklist.h"

#include "ilist_updateinplace.h"

#include <cassert>

#include "pmallocator.h"
#include "stdallocator.h"
#include "arenaallocator.h"
#include "simulatedarenaallocator.h"

#include "software_prefetch.h"

namespace hm4{

template<class T_Allocator>
struct UnrolledLinkList<T_Allocator>::Node{
	UnrolledLinkList::MyPairVector	data;
	Node			*next = nullptr;

	int cmp(std::string_view const key) const{
		return data.back().cmp(key);
	}

	constexpr void prefetch() const{
		constexpr bool use_prefetch = true;

		if constexpr(use_prefetch){
			builtin_prefetch(& this->data.back(), 0, 1);
			builtin_prefetch(this->next, 0, 1);
		}
	}

	constexpr static auto begin_or_null(const Node *node){
		using It = typename UnrolledLinkList::MyPairVector::iterator;

		if (node)
			return node->data.begin();
		else
			return It{};
	}
};

template<class T_Allocator>
struct UnrolledLinkList<T_Allocator>::NodeLocator{
	Node	**prev;
	Node	*node;
	bool	found	= false;
};

namespace{
	// we not really need to check the integrity of the list
	constexpr bool corruptionCheck = false;

	[[maybe_unused]]
	void corruptionExit(){
		fprintf(stderr, "====================================\n");
		fprintf(stderr, "=== Detected UnrolledLinkList corruption ===\n");
		fprintf(stderr, "====================================\n");
		exit(100);
	}
}

// ==============================

template<class T_Allocator>
UnrolledLinkList<T_Allocator>::UnrolledLinkList(Allocator &allocator) : allocator_(& allocator){
	zeroing_();
}

template<class T_Allocator>
UnrolledLinkList<T_Allocator>::UnrolledLinkList(UnrolledLinkList &&other):
			head_		(std::move(other.head_		)),
			lc_		(std::move(other.lc_		)),
			allocator_	(std::move(other.allocator_	)){
	other.zeroing_();
}

template<class T_Allocator>
void UnrolledLinkList<T_Allocator>::deallocate_(Node *node){
	using namespace MyAllocator;

	node->data.destruct(getAllocator());
	deallocate(getAllocator(), node);
}

template<class T_Allocator>
void UnrolledLinkList<T_Allocator>::zeroing_(){
	lc_.clr();

	head_ = nullptr;
}

template<class T_Allocator>
bool UnrolledLinkList<T_Allocator>::clear(){
	if (allocator_->reset() == false){
		for(Node *node = head_; node; ){
			node->prefetch();

			Node *copy = node;

			node = node->next;

			deallocate_(copy);
		}
	}

	zeroing_();

	return true;
}

template<class T_Allocator>
void UnrolledLinkList<T_Allocator>::print() const{
	printf("==begin list==\n");

	for(const Node *node = head_; node; node = node->next){
		printf("Node: %p\n", (void *) node);

		printf("--begin data--\n");
		for(auto &x : node->data)
			x.print();

		printf("---end data---\n");
	}

	printf("===end list===\n\n\n\n\n");
}

template<class T_Allocator>
auto UnrolledLinkList<T_Allocator>::fix_iterator_(const Node *node, typename UnrolledLinkList::MyPairVector::iterator it) const -> iterator{
	if (it != node->data.end())
		return { node, it };

	if (!node->next)
		return end();

	node = node->next;
	return { node, node->data.begin() };
};

template<class T_Allocator>
auto UnrolledLinkList<T_Allocator>::fix_iterator_(const Node *node, typename UnrolledLinkList::MyPairVector::const_ptr_iterator it) const -> iterator{
	return fix_iterator_(node, typename UnrolledLinkList::MyPairVector::iterator(it));
}

template<class T_Allocator>
template<class PFactory>
auto UnrolledLinkList<T_Allocator>::insertF(PFactory &factory) -> iterator{
	auto constructNode = [](auto &allocator) -> Node *{
		using namespace MyAllocator;
		Node *newnode = allocate<Node>(allocator);

		if (!newnode)
			return nullptr;

		newnode->data.construct();
		newnode->next = nullptr;

		return newnode;
	};

	auto const &key = factory.getKey();

	const auto nl = locate_(key);

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

		Node *newnode = constructNode(getAllocator());

		if (!newnode)
			return end();

		// connect node
		newnode->next = std::exchange(*nl.prev, newnode);

		// These is a small problem here:
		// If insertF() fails, we will have empty node.
		// However it will be only one in the list,
		// so should be OK.

		auto const it = newnode->data.insertF(factory, getAllocator(), lc_);

		return fix_iterator_(
			newnode,
			it
		);
	}

	if (node->data.full()){
		// current node is full, make new one and split elements.


		Node *newnode = constructNode(getAllocator());

		if (!newnode)
			return end();

		// connect node after
		newnode->next = std::exchange(node->next, newnode);

		node->data.split(newnode->data);

		// is unclear where the pair should go
		// also we have knowledge how the node is split

		int const cmp = nl.node->cmp(key);

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
bool UnrolledLinkList<T_Allocator>::erase_(std::string_view const key){
	// better Pair::check(key), but might fail because of the caller.
	assert(!key.empty());

	auto nl = locate_(key);

	if (!nl.node)
		return false;

	if constexpr(corruptionCheck)
		if (*nl.prev != nl.node)
			corruptionExit();

	if (!nl.node->data.erase_(key, getAllocator(), lc_))
		return false;

	if (nl.node->data.size())
		return true;

	// node is zero size, it must be removed

	*nl.prev = nl.node->next;

	deallocate_(nl.node);

	return true;
}

// ==============================

template<class T_Allocator>
auto UnrolledLinkList<T_Allocator>::locate_(std::string_view const key) -> NodeLocator{
	// better Pair::check(key), but might fail because of the caller.
	assert(!key.empty());

	Node **jtable = & head_;

	for(Node *node = *jtable; node; node = node->next){
		node->prefetch();

		int cmp = node->cmp(key);

		if (cmp >= 0){
			return { jtable, node, cmp == 0 };
		}else if (!node->next){
			// this is the last node
			return { jtable, node };
		}

		jtable = & node->next;
	}

	// list seems to be empty
	return { jtable, nullptr };
}

template<class T_Allocator>
template<bool ExactMatch>
auto UnrolledLinkList<T_Allocator>::find(std::string_view const key, std::bool_constant<ExactMatch>) const -> iterator{
	assert(!key.empty());

	const Node *node;
	int cmp = 0;

	for(node = head_; node; node = node->next){
		node->prefetch();

		cmp = node->cmp(key);

		if (cmp >= 0)
			break;
	}

	if (!node)
		return end();

	if (cmp == 0){
		// miracle, direct hit
		return { node, node->data.end() - 1 };
	}

	// search inside node

	auto const &[found, it] = node->data.locateC_(key);

	if constexpr(ExactMatch)
		return found ? iterator{ node, it } : end();

	return fix_iterator_(node, it);
}

// ==============================


template<class T_Allocator>
auto UnrolledLinkList<T_Allocator>::iterator::operator++() -> iterator &{
	if (++it_ != node_->data.end())
		return *this;

	node_	= node_->next;
	it_	= Node::begin_or_null(node_);

	return *this;
}

template<class T_Allocator>
const Pair &UnrolledLinkList<T_Allocator>::iterator::operator*() const{
	return *it_;
}

template<class T_Allocator>
auto UnrolledLinkList<T_Allocator>::begin() const -> iterator{
	return { head_, Node::begin_or_null(head_) };
}

// ==============================

template class UnrolledLinkList<MyAllocator::PMAllocator>;
template class UnrolledLinkList<MyAllocator::STDAllocator>;
template class UnrolledLinkList<MyAllocator::ArenaAllocator>;
template class UnrolledLinkList<MyAllocator::SimulatedArenaAllocator>;

template auto UnrolledLinkList<MyAllocator::PMAllocator>		::find(std::string_view const key, std::true_type ) const -> iterator;
template auto UnrolledLinkList<MyAllocator::STDAllocator>		::find(std::string_view const key, std::true_type ) const -> iterator;
template auto UnrolledLinkList<MyAllocator::ArenaAllocator>		::find(std::string_view const key, std::true_type ) const -> iterator;
template auto UnrolledLinkList<MyAllocator::SimulatedArenaAllocator>	::find(std::string_view const key, std::true_type ) const -> iterator;

template auto UnrolledLinkList<MyAllocator::PMAllocator>		::find(std::string_view const key, std::false_type) const -> iterator;
template auto UnrolledLinkList<MyAllocator::STDAllocator>		::find(std::string_view const key, std::false_type) const -> iterator;
template auto UnrolledLinkList<MyAllocator::ArenaAllocator>		::find(std::string_view const key, std::false_type) const -> iterator;
template auto UnrolledLinkList<MyAllocator::SimulatedArenaAllocator>	::find(std::string_view const key, std::false_type) const -> iterator;

template auto UnrolledLinkList<MyAllocator::PMAllocator>		::insertF(PairFactory::Normal		&factory) -> iterator;
template auto UnrolledLinkList<MyAllocator::STDAllocator>		::insertF(PairFactory::Normal		&factory) -> iterator;
template auto UnrolledLinkList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Normal		&factory) -> iterator;
template auto UnrolledLinkList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Normal		&factory) -> iterator;

template auto UnrolledLinkList<MyAllocator::PMAllocator>		::insertF(PairFactory::Expires		&factory) -> iterator;
template auto UnrolledLinkList<MyAllocator::STDAllocator>		::insertF(PairFactory::Expires		&factory) -> iterator;
template auto UnrolledLinkList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Expires		&factory) -> iterator;
template auto UnrolledLinkList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Expires		&factory) -> iterator;

template auto UnrolledLinkList<MyAllocator::PMAllocator>		::insertF(PairFactory::Tombstone	&factory) -> iterator;
template auto UnrolledLinkList<MyAllocator::STDAllocator>		::insertF(PairFactory::Tombstone	&factory) -> iterator;
template auto UnrolledLinkList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Tombstone	&factory) -> iterator;
template auto UnrolledLinkList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Tombstone	&factory) -> iterator;

template auto UnrolledLinkList<MyAllocator::PMAllocator>		::insertF(PairFactory::Clone		&factory) -> iterator;
template auto UnrolledLinkList<MyAllocator::STDAllocator>		::insertF(PairFactory::Clone		&factory) -> iterator;
template auto UnrolledLinkList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Clone		&factory) -> iterator;
template auto UnrolledLinkList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Clone		&factory) -> iterator;

template auto UnrolledLinkList<MyAllocator::PMAllocator>		::insertF(PairFactory::IFactory		&factory) -> iterator;
template auto UnrolledLinkList<MyAllocator::STDAllocator>		::insertF(PairFactory::IFactory		&factory) -> iterator;
template auto UnrolledLinkList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::IFactory		&factory) -> iterator;
template auto UnrolledLinkList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::IFactory		&factory) -> iterator;

} // namespace


