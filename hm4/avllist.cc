#include "avllist.h"

#include <cstdint>
#include <cassert>
#include <algorithm>	// max, swap

#include "ilist_updateinplace.h"

#include "pmallocator.h"
#include "stdallocator.h"
#include "arenaallocator.h"
#include "simulatedarenaallocator.h"

namespace hm4{



template<class T_Allocator>
struct AVLList<T_Allocator>::Node{
	using balance_t = int8_t;

	balance_t	balance	= 0;		// 1 + 7!!! gap
	Node		*l	= nullptr;	// 8
	Node		*r	= nullptr;	// 8
	Node		*p	= nullptr;	// 8
	Pair		data;			// multiple

	constexpr void clear(Node *parent){
		balance	= 0;
		l	= nullptr;
		r	= nullptr;
		p	= parent;
	}

	int cmp(std::string_view const key) const{
		return data.cmp(key);
	}
};



// ==============================



namespace avl_impl_{

	// works for const Node * too
	template<class Node>
	Node *getMinValueNode(Node *node){
		assert(node);

		while(node->l)
			node = node->l;

		return node;
	}

	// works for const Node * too
	template<class Node>
	Node *getSuccessorNode(Node *node){
		assert(node);

		// find node successor

		if (node->r){
			// go right
			return getMinValueNode(node->r);
		}

		// go up
		Node *parent = node->p;

		while(parent && node == parent->r){
			node = parent;
			parent = parent->p;
		}

		return parent;
	}

	template<class Node>
	const Node *findFix(const Node *node, std::string_view key){
		while(node){
			int const cmp = node->cmp(key);

			if (cmp < 0)
				node = node->p;
			else
				break;
		}

		return node;
	}

	template<class Node>
	void swapLinks(Node *a, Node *b){
		assert(a);
		assert(b);

		using std::swap;

		swap(a->balance	, b->balance	);
		swap(a->l	, b->l		);
		swap(a->r	, b->r		);
		swap(a->p	, b->p		);

		// fix parents, easy because we have parent link

		auto fixParent = [](Node *node, const Node *original){
			if (node){
				auto *parent = node->p;
				if (parent->l == original)
					parent->l = node;
				else
					parent->r = node;
			}
		};

		fixParent(a, b);
		fixParent(b, a);
	}
}

template<class T_Allocator>
size_t AVLList<T_Allocator>::height() const{
	size_t height = 0;

	const auto *node = root_;

	while(node){
		++height;

		if (node->balance < 0)
			node = node->l;
		else
			node = node->r;
	}

	return height;
}



// ==============================



template<class T_Allocator>
bool AVLList<T_Allocator>::erase_(std::string_view const key){
	auto *node = root_;

	while(node){
		int const cmp = node->cmp(key);

		if (cmp > 0){
			node = node->l;
			continue;
		}

		if (cmp < 0){
			node = node->r;
			continue;
		}

		break;
	}

	if (!node)
		return false;

	if (node->l && node->r){
		// CASE 3 - node two children
		auto *successor = avl_impl_::getMinValueNode(node->r);

		avl_impl_::swapLinks(node, successor);

		// Silently proceed to CASE 2
		node = successor;
	}

	if (auto *child = node->l ? node->l : node->r; child){
		// CASE 2: node with only one child
		child->p = node->p;

		if (!node->p){
			lc_.dec(node->data.bytes());
			deallocate_(node);

			this->root_ = child;

			return true;
		}

		if (auto *parent = node->p; node == parent->l){
			parent->l = child;
			++parent->balance;

			lc_.dec(node->data.bytes());
			deallocate_(node);

			if (parent->balance == +1){
				return true;
			}else{
				rebalanceAfterErase_(parent);
				return true;
			}
		}else{ // node == parent->r
			parent->r = child;
			--parent->balance;

			lc_.dec(node->data.bytes());
			deallocate_(node);

			if (parent->balance == -1){
				return true;
			}else{
				rebalanceAfterErase_(parent);
				return true;
			}
		}
	}

	// CASE 1: node with no children

	if (!node->p){
		lc_.dec(node->data.bytes());
		deallocate_(node);

		this->root_ = nullptr;

		return true;
	}

	if (auto *parent = node->p; node == parent->l){
		parent->l = nullptr;
		++parent->balance;

		lc_.dec(node->data.bytes());
		deallocate_(node);

		if (parent->balance == +1){
			return true;
		}else{
			rebalanceAfterErase_(parent);
			return true;
		}
	}else{ // node == parent->r
		parent->r = nullptr;
		--parent->balance;

		lc_.dec(node->data.bytes());
		deallocate_(node);

		if (parent->balance == -1){
			return true;
		}else{
			rebalanceAfterErase_(parent);
			return true;
		}
	}
}



namespace avl_impl_{
	template<class Node, class T_Allocator, class PFactory>
	Node *allocateNode(T_Allocator &allocator, PFactory &factory, ListCounter &lc, Node *parent){
		size_t const bytes = factory.bytes();
		size_t const nodeSize = sizeof(Node) - sizeof(Pair) + bytes;

		using namespace MyAllocator;
		Node *node = allocate<Node>(allocator, nodeSize);

		if (node == nullptr)
			return nullptr;

		node->clear(parent);

		factory.create(& node->data);

		size_t const size = bytes;

		lc.inc(size);

		return node;
	}
}

template<class T_Allocator>
template<class PFactory>
auto AVLList<T_Allocator>::insertF(PFactory &factory) -> iterator{
	if (!root_){
		// tree is empty.
		// insert, no balance.
		auto *newnode = avl_impl_::allocateNode<Node>(getAllocator(), factory, lc_, nullptr);

		if (!newnode)
			return this->end();

		root_ = newnode;

		return root_;
	}

	auto const &key = factory.getKey();

	Node *node   = root_;

	while(true){
		int const cmp = node->cmp(key);

		if (cmp > 0){
			if (!node->l){
				using namespace MyAllocator;
				Node *newnode = avl_impl_::allocateNode<Node>(getAllocator(), factory, lc_, node);

				if (!newnode)
					return this->end();

				node->l = newnode;
				--node->balance;
				rebalanceAfterInsert_(node);

				return newnode;
			}else{
				node = node->l;
				continue;
			}
		}

		if (cmp < 0){
			if (!node->r){
				auto newnode = avl_impl_::allocateNode(getAllocator(), factory, lc_, node);

				if (!newnode)
					return this->end();

				node->r = newnode;
				++node->balance;
				rebalanceAfterInsert_(node);

				return newnode;
			}else{
				node = node->r;
				continue;
			}
		}

		if (cmp == 0){
			// update node in place.

			Pair *olddata = & node->data;

			// check if we can update

			if constexpr(config::LIST_CHECK_PAIR_FOR_REPLACE)
				if (!isValidForReplace(factory.getCreated(), *olddata))
					return this->end();

			// try update pair in place.
			if (tryUpdateInPlaceLC(getAllocator(), olddata, factory, lc_)){
				// successfully updated.
				return node;
			}

			// allocate new node and change links
			Node *newnode = avl_impl_::allocateNode(getAllocator(), factory, lc_, node);

			if (!newnode)
				return this->end();

			avl_impl_::swapLinks(newnode, node);

			lc_.dec( olddata->bytes() );

			// deallocate old node
			using namespace MyAllocator;
			deallocate(allocator_, node);

			return newnode;
		}
	}

	// never reach here.
}



template<class T_Allocator>
template<bool ExactEvaluation>
auto AVLList<T_Allocator>::find(std::string_view const key, std::bool_constant<ExactEvaluation>) const -> iterator{
	assert(!key.empty());

	using avl_impl_::findFix;

	auto *node = root_;

	while(node){
		int const cmp = node->cmp(key);

		if (cmp > 0){
			if constexpr(!ExactEvaluation)
				if (node->l == nullptr)
					return findFix(node, key);

			node = node->l;
			continue;
		}

		if (cmp < 0){
			if constexpr(!ExactEvaluation)
				if (node->r == nullptr)
					return findFix(node, key);

			node = node->r;
			continue;
		}

		break;
	}

	return node;
}



template<class T_Allocator>
void AVLList<T_Allocator>::deallocate_(Node *node){
	using namespace MyAllocator;
	deallocate(allocator_, node);
}

template<class T_Allocator>
void AVLList<T_Allocator>::deallocateTree_(Node *node){
	if (allocator_->reset() == false){
		// seems there is no viable alternative,
		// but recursion

		if (!node)
			return;

		deallocateTree_(node->l);
		deallocateTree_(node->r);
		deallocate_(node);
	}
}



// ROTATIONS

template<class T_Allocator>
void AVLList<T_Allocator>::rotateL_(Node *n){
	/*
	 *     n             r
	 *      \           /
	 *       r   ==>   n
	 *      /           \
	 *     t             t
	 */

	auto *r = n->r;
	auto *t = r->l;
	n->r = t;

	if (t)
		t->p = n;

	r->p = n->p;

	if (!n->p)
		this->root_ = r;
	else if (n->p->l == n)
		n->p->l = r;
	else
		n->p->r = r;

	r->l = n;
	n->p = r;
}

template<class T_Allocator>
void AVLList<T_Allocator>::rotateR_(Node *n){
	/*
	 *     n             l
	 *    /               \
	 *   l       ==>       n
	 *    \               /
	 *     t             t
	 */

	auto *l = n->l;
	auto *t = l->r;
	n->l = t;

	if (t)
		t->p = n;

	l->p = n->p;

	if (!n->p)
		this->root_ = l;
	else if (n->p->r == n)
		n->p->r = l;
	else
		n->p->l = l;

	l->r = n;
	n->p = l;
}

template<class T_Allocator>
void AVLList<T_Allocator>::rotateRL_(Node *node){
	rotateR_(node->r);
	rotateL_(node);
}

template<class T_Allocator>
void AVLList<T_Allocator>::rotateLR_(Node *node){
	rotateL_(node->l);
	rotateR_(node);
}

template<class T_Allocator>
void AVLList<T_Allocator>::rebalanceAfterInsert_(Node *node){
	assert(node);

	while(node->balance){
		if (node->balance == +2){
			// right heavy
			if (node->r->balance == +1){
				node->balance = 0;
				node->r->balance = 0;

				rotateL_(node);
			}else{ // node->r->balance == -1
				auto const rlBalance = node->r->l->balance;

				node->r->l->balance = 0;
				node->r->balance = 0;
				node->balance = 0;

				if (rlBalance == +1)
					node->balance = -1;
				else if (rlBalance == -1)
					node->r->balance = +1;

				rotateRL_(node);
			}

			break;
		}

		if (node->balance == -2){
			// left heavy
			if (node->l->balance == -1){
				node->balance = 0;
				node->l->balance = 0;

				rotateR_(node);
			}else{ // node->r->balance == +1
				auto const lrBalance = node->l->r->balance;

				node->l->r->balance = 0;
				node->l->balance = 0;
				node->balance = 0;

				if (lrBalance == -1)
					node->balance = +1;
				else if (lrBalance == +1)
					node->l->balance = -1;

				rotateLR_(node);
			}

			break;
		}

		auto *parent = node->p;

		if (!parent)
			return;

		if (parent->l == node)
			--parent->balance;
		else
			++parent->balance;

		node = node->p;
	}
}

template<class T_Allocator>
void AVLList<T_Allocator>::rebalanceAfterErase_(Node *node){
	assert(node);

	while(true){
		if (node->balance == +2){
			// right heavy

			if (node->r->balance == +1){
				node->balance = 0;
				node->r->balance = 0;

				rotateL_(node);
			}else if(node->r->balance == 0){
				node->balance = +1;
				node->r->balance = -1;

				rotateL_(node);
			}else{ // node->r->balance == -1
				auto const rlBalance = node->r->l->balance;

				node->r->l->balance = 0;
				node->r->balance = 0;
				node->balance = 0;

				if (rlBalance == +1)
					node->balance = -1;
				else if (rlBalance == -1)
					node->r->balance = +1;

				rotateRL_(node);
			}

			node = node->p;
		}else
		if (node->balance == -2){
			// left heavy

			if (node->l->balance == -1){
				node->balance = 0;
				node->r->balance = 0;

				rotateR_(node);
			}else if(node->l->balance == 0){
				node->balance = -1;
				node->l->balance = +1;

				rotateR_(node);
			}else{ // node->l->balance == +1
				auto const lrBalance = node->l->r->balance;

				node->l->r->balance = 0;
				node->l->balance = 0;
				node->balance = 0;

				if (lrBalance == -1)
					node->balance = 1;
				else if (lrBalance == +1)
					node->l->balance = -1;

				rotateLR_(node);
			}

			node = node->p;
		}

		auto *parent = node->p;

		if (!parent)
			return;

		if (node == parent->l){
			++parent->balance;

			if (parent->balance == +1)
				return;
		}else{ // node == parent->r
			--parent->balance;

			if (parent->balance == -1)
				return;
		}

		node = node->p;
	}
}



// ==============================



namespace avl_impl_{

	template<typename Node>
	int testALVTreeIntegrity_height(const Node *node){
		if (!node)
			return 0;

		return std::max(
			testALVTreeIntegrity_height(node->l),
			testALVTreeIntegrity_height(node->r)
		) + 1;
	}

	template<bool CheckHeight = false, typename Node>
	void testALVTreeIntegrity(const Node *node, const Node *parent = nullptr){
		// not important, so it stay recursive.

		if (!node)
			return;

		assert(node->p == parent);
		assert(node->balance >= -1 && node->balance <= +1);

		if constexpr(CheckHeight){
			auto const balance = testALVTreeIntegrity_height(node->r) - testALVTreeIntegrity_height(node->l);
			assert(balance >= -1 && balance <= +1);
			assert(balance == node->balance);
		}

		if (node->l)
			assert(node->l->data.getKey() < node->data.getKey());

		if (node->r)
			assert(node->r->data.getKey() > node->data.getKey());

		testALVTreeIntegrity(node->l, node);
		testALVTreeIntegrity(node->r, node);
	}

} // namespace avl_impl_

template<class T_Allocator>
void AVLList<T_Allocator>::testALVTreeIntegrity(std::false_type) const{
	return avl_impl_::testALVTreeIntegrity<0>(root_);
}

template<class T_Allocator>
void AVLList<T_Allocator>::testALVTreeIntegrity(std::true_type) const{
	return avl_impl_::testALVTreeIntegrity<1>(root_);
}



// ==============================



template<class T_Allocator>
auto AVLList<T_Allocator>::begin() const -> iterator{
	if (root_)
		return avl_impl_::getMinValueNode(root_);
	else
		return nullptr;
}



template<class T_Allocator>
auto AVLList<T_Allocator>::iterator::operator++() -> iterator &{
	node = avl_impl_::getSuccessorNode(node);

	return *this;
}



template<class T_Allocator>
const Pair &AVLList<T_Allocator>::iterator::operator*() const{
	return node->data;
}

// ==============================

template class AVLList<MyAllocator::PMAllocator>;
template class AVLList<MyAllocator::STDAllocator>;
template class AVLList<MyAllocator::ArenaAllocator>;
template class AVLList<MyAllocator::SimulatedArenaAllocator>;

template auto AVLList<MyAllocator::PMAllocator>			::find(std::string_view const key, std::true_type ) const -> iterator;
template auto AVLList<MyAllocator::STDAllocator>		::find(std::string_view const key, std::true_type ) const -> iterator;
template auto AVLList<MyAllocator::ArenaAllocator>		::find(std::string_view const key, std::true_type ) const -> iterator;
template auto AVLList<MyAllocator::SimulatedArenaAllocator>	::find(std::string_view const key, std::true_type ) const -> iterator;

template auto AVLList<MyAllocator::PMAllocator>			::find(std::string_view const key, std::false_type) const -> iterator;
template auto AVLList<MyAllocator::STDAllocator>		::find(std::string_view const key, std::false_type) const -> iterator;
template auto AVLList<MyAllocator::ArenaAllocator>		::find(std::string_view const key, std::false_type) const -> iterator;
template auto AVLList<MyAllocator::SimulatedArenaAllocator>	::find(std::string_view const key, std::false_type) const -> iterator;

template auto AVLList<MyAllocator::PMAllocator>			::insertF(PairFactory::Normal		&factory) -> iterator;
template auto AVLList<MyAllocator::STDAllocator>		::insertF(PairFactory::Normal		&factory) -> iterator;
template auto AVLList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Normal		&factory) -> iterator;
template auto AVLList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Normal		&factory) -> iterator;

template auto AVLList<MyAllocator::PMAllocator>			::insertF(PairFactory::Expires		&factory) -> iterator;
template auto AVLList<MyAllocator::STDAllocator>		::insertF(PairFactory::Expires		&factory) -> iterator;
template auto AVLList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Expires		&factory) -> iterator;
template auto AVLList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Expires		&factory) -> iterator;

template auto AVLList<MyAllocator::PMAllocator>			::insertF(PairFactory::Tombstone	&factory) -> iterator;
template auto AVLList<MyAllocator::STDAllocator>		::insertF(PairFactory::Tombstone	&factory) -> iterator;
template auto AVLList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Tombstone	&factory) -> iterator;
template auto AVLList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Tombstone	&factory) -> iterator;

template auto AVLList<MyAllocator::PMAllocator>			::insertF(PairFactory::Clone		&factory) -> iterator;
template auto AVLList<MyAllocator::STDAllocator>		::insertF(PairFactory::Clone		&factory) -> iterator;
template auto AVLList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Clone		&factory) -> iterator;
template auto AVLList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Clone		&factory) -> iterator;

template auto AVLList<MyAllocator::PMAllocator>			::insertF(PairFactory::IFactory		&factory) -> iterator;
template auto AVLList<MyAllocator::STDAllocator>		::insertF(PairFactory::IFactory		&factory) -> iterator;
template auto AVLList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::IFactory		&factory) -> iterator;
template auto AVLList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::IFactory		&factory) -> iterator;

} // namespace hm4

