#include "avllist.h"

#include <cstdint>
#include <cassert>
#include <algorithm>	// max, swap

#include "ilist/updateinplace.h"

#include "pmallocator.h"
#include "stdallocator.h"
#include "arenaallocator.h"
#include "simulatedarenaallocator.h"

namespace hm4{



template<class T_Allocator>
struct AVLList<T_Allocator>::Node{
	Node		*l;	// 8
	Node		*r;	// 8
	Node		*p_;	// 8
	Pair		data;	// multiple

public:
	constexpr void clear(Node *parent){
		l	= nullptr;
		r	= nullptr;
		p_	= setBP0__(parent);
	}

	int cmp(std::string_view const key) const{
		return data.cmp(key);
	}

public:
	constexpr auto *getP(){
		auto const pu = reinterpret_cast<uintptr_t>(p_) & ~mask;
		return reinterpret_cast<Node *>(pu);
	}

	constexpr const auto *getP() const{
		auto const pu = reinterpret_cast<uintptr_t>(p_) & ~mask;
		return reinterpret_cast<Node *>(pu);
	}

	constexpr void setP(Node *node){
		auto const bu = reinterpret_cast<uintptr_t>(p_  ) &  mask;
		auto const pu = reinterpret_cast<uintptr_t>(node) & ~mask;

		p_ = reinterpret_cast<Node *>(bu | pu);
	}

	constexpr int getBalance() const{
		auto const bu = reinterpret_cast<uintptr_t>(p_) & mask;

		return static_cast<int>(bu) - off;
	}

	constexpr int setBalance(int balance){
		assert(balance >= -2 && balance <= +2);

		auto const bu = static_cast     <uintptr_t>(balance + off) &  mask;
		auto const pu = reinterpret_cast<uintptr_t>(p_           ) & ~mask;

		p_ = reinterpret_cast<Node *>(bu | pu);

		return balance;
	}

	constexpr void incBalance(){
		setBalance(getBalance() + 1);
	}

	constexpr void decBalance(){
		setBalance(getBalance() - 1);
	}

private:
	[[maybe_unused]]
	constexpr static Node *BP0__(){
		uintptr_t const bu = off &  mask;
		uintptr_t const pu = 0;

		return reinterpret_cast<Node *>(bu | pu);
	}

	constexpr static Node *setBP0__(Node *node){
		auto const bu = static_cast     <uintptr_t>(off ) &  mask;
		auto const pu = reinterpret_cast<uintptr_t>(node) & ~mask;

		return reinterpret_cast<Node *>(bu | pu);
	}

private:
	constexpr static uintptr_t mask = 0b000111;
	constexpr static int       off  = 2;
};



// ==============================



namespace avl_impl_{

	// works for const Node * too
	template<class Node>
	Node *getMaxValueNode(Node *node){
		assert(node);

		while(node->r)
			node = node->r;

		return node;
	}

	// works for const Node * too
	template<class Node>
	Node *getPredecessorNode(Node *node){
		assert(node);

		// find node successor

		if (node->l){
			// go right
			return getMaxValueNode(node->l);
		}

		// go up
		Node *parent = node->getP();

		while(parent && node == parent->l){
			node = parent;
			parent = parent->getP();
		}

		return parent;
	}

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
		Node *parent = node->getP();

		while(parent && node == parent->r){
			node = parent;
			parent = parent->getP();
		}

		return parent;
	}
}

template<class T_Allocator>
size_t AVLList<T_Allocator>::height() const{
	size_t height = 0;

	const auto *node = root_;

	while(node){
		++height;

		if (node->getBalance() < 0)
			node = node->l;
		else
			node = node->r;
	}

	return height;
}

template<class T_Allocator>
size_t const AVLList<T_Allocator>::INTERNAL_NODE_SIZE = checkInternalNodeSize<
							sizeof(Node) - sizeof(Pair)
						>();

// ==============================



template<class T_Allocator>
InsertResult AVLList<T_Allocator>::erase_(std::string_view const key){
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
		return InsertResult::skipDeleted();

	if (node->l && node->r){
		// CASE 3 - node two children
		auto *successor = avl_impl_::getMinValueNode(node->r);

		// I know successor left child is nullptr
		// but erase is slow and not really used anyway
		swapLinks_(node, successor);

		// Silently proceed to CASE 2

		// counter intuitive, but we do not need this line,
		// since successor is now in the place of node.
		// node = successor;
	}

	if (auto *child = node->l ? node->l : node->r; child){
		// CASE 2: node with only one child
		child->setP( node->getP() );

		// do not use fixParentAndChildren_(), because it does more work.

		if (!node->getP()){
			lc_.dec(node->data.bytes());
			deallocate_(node);

			this->root_ = child;

			return InsertResult::deleted();
		}

		if (auto *parent = node->getP(); node == parent->l){
			parent->l = child;
			parent->incBalance();

			lc_.dec(node->data.bytes());
			deallocate_(node);

			if (parent->setBalance(+1)){
				return InsertResult::deleted();
			}else{
				rebalanceAfterErase_(parent);
				return InsertResult::deleted();
			}
		}else{ // node == parent->r
			parent->r = child;
			parent->decBalance();

			lc_.dec(node->data.bytes());
			deallocate_(node);

			if (parent->setBalance(-1)){
				return InsertResult::deleted();
			}else{
				rebalanceAfterErase_(parent);
				return InsertResult::deleted();
			}
		}
	}

	// CASE 1: node with no children

	if (!node->getP()){
		lc_.dec(node->data.bytes());
		deallocate_(node);

		this->root_ = nullptr;

		return InsertResult::deleted();
	}

	if (auto *parent = node->getP(); node == parent->l){
		parent->l = nullptr;
		parent->incBalance();

		lc_.dec(node->data.bytes());
		deallocate_(node);

		if (parent->getBalance() == +1){
			return InsertResult::deleted();
		}else{
			rebalanceAfterErase_(parent);
			return InsertResult::deleted();
		}
	}else{ // node == parent->r
		parent->r = nullptr;
		parent->decBalance();

		lc_.dec(node->data.bytes());
		deallocate_(node);

		if (parent->getBalance() == -1){
			return InsertResult::deleted();
		}else{
			rebalanceAfterErase_(parent);
			return InsertResult::deleted();
		}
	}
}



namespace avl_impl_{
	template<class Node, class T_Allocator, class PFactory>
	Node *allocateNode(T_Allocator &allocator, PFactory &factory, ListCounter &lc, Node *parent){
		if (!factory.valid())
			return nullptr;

		size_t const bytes = factory.bytes();
		size_t const nodeSize = sizeof(Node) - sizeof(Pair) + bytes;

		using namespace MyAllocator;
		Node *node = allocate<Node>(allocator, nodeSize);

		if (node == nullptr)
			return nullptr;

		node->clear(parent);

		factory.create(& node->data);

		lc.inc(bytes);

		return node;
	}
}

template<class T_Allocator>
template<class PFactory>
auto AVLList<T_Allocator>::insertF(PFactory &factory) -> InsertResult{
	if (!factory.valid())
		return InsertResult::errorInvalid();

	if (!root_){
		// tree is empty.
		// insert, no balance.
		auto *newnode = avl_impl_::allocateNode<Node>(getAllocator(), factory, lc_, nullptr);

		if (!newnode)
			return InsertResult::errorNoMemory();

		root_ = newnode;

		return InsertResult::inserted( & newnode->data );
	}

	auto const &key = factory.getKey();

	Node *node   = root_;

	while(true){
		int const cmp = node->cmp(key);

		if (cmp > 0){
			if (!node->l){
				auto *newnode = avl_impl_::allocateNode(getAllocator(), factory, lc_, node);

				if (!newnode)
					return InsertResult::errorNoMemory();

				node->l = newnode;
				node->decBalance();
				rebalanceAfterInsert_(node);

				return InsertResult::inserted( & newnode->data );
			}else{
				node = node->l;
				continue;
			}
		}

		if (cmp < 0){
			if (!node->r){
				auto *newnode = avl_impl_::allocateNode(getAllocator(), factory, lc_, node);

				if (!newnode)
					return InsertResult::errorNoMemory();

				node->r = newnode;
				node->incBalance();
				rebalanceAfterInsert_(node);

				return InsertResult::inserted( & newnode->data );
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
					return InsertResult::skipInserted();

			// try update pair in place.
			if (tryUpdateInPlaceLC(getAllocator(), olddata, factory, lc_)){
				// successfully updated.
				return InsertResult::updatedInPlace(olddata);
			}

			// allocate new node and change links
			Node *newnode = avl_impl_::allocateNode(getAllocator(), factory, lc_, node);

			if (!newnode)
				return InsertResult::errorNoMemory();

			copyLinks_(newnode, node);

			lc_.dec( olddata->bytes() );

			// deallocate old node
			using namespace MyAllocator;
			deallocate(allocator_, node);

			return InsertResult::replaced( & newnode->data );
		}
	}

	// never reach here.
}



template<class T_Allocator>
auto AVLList<T_Allocator>::find(std::string_view const key) const -> iterator{
	assert(!key.empty());

	auto *node = root_;

	while(node){
		int const cmp = node->cmp(key);

		if (cmp > 0){
			if (node->l == nullptr){
				// We need successor of the `key`,
				// but it should be on the left, but is not there.
				// this means the `node` is the successor of the `key`.
				return { node, root_ };
			}

			node = node->l;
			continue;
		}

		if (cmp < 0){
			if (node->r == nullptr){
				// We need successor of the `key`,
				// this means the successor of the `node`,
				// is also the successor of the `key`.
				return {
					avl_impl_::getSuccessorNode(node),
					root_
				};
			}

			node = node->r;
			continue;
		}

		break;
	}

	return { node, root_ };
}



template<class T_Allocator>
const Pair *AVLList<T_Allocator>::findExact(std::string_view const key) const{
	assert(!key.empty());

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

		// found
		return & node->data;
	}

	// not found
	return nullptr;
}



template<class T_Allocator>
void AVLList<T_Allocator>::deallocate_(Node *node){
	using namespace MyAllocator;
	deallocate(allocator_, node);
}

template<class T_Allocator>
void AVLList<T_Allocator>::deallocateTree_(Node *node){
	if (allocator_->reset())
		return;

	// Morris traversal
	// https://gist.github.com/eopXD/7e7c86ee0662632df79289023ab0b47a

	while(node){
		if (!node->l){
			Node *tmp = node;
			node = node->r;
			deallocate_(tmp);
		}else{
			Node *tmp = node->l;

			while(tmp->r and tmp->r != node )
				tmp = tmp->r;

			if (!tmp->r){
				tmp->r = node;
				tmp  = node;
				node = node->l;
				tmp->l = nullptr;
			}
			// else{
			//	assert(0);
			// }
		}
	}
}



// ROTATIONS

template<class T_Allocator>
void AVLList<T_Allocator>::rotateL_(Node *n){
	/*
	 *   n             r
	 *    \           /
	 *     r   ==>   n
	 *    /           \
	 *   t*            t*
	 */

	auto *r = n->r;
	auto *t = r->l;

	n->r = t;
	if (t)
		t->setP(n);

	r->l = n;
	r->setP( n->getP() );

	fixParentAndChildren_<1,1,0>(r, n);
}

template<class T_Allocator>
void AVLList<T_Allocator>::rotateR_(Node *n){
	/*
	 *     n         l
	 *    /           \
	 *   l     ==>     n
	 *    \           /
	 *     t*        t*
	 */

	auto *l = n->l;
	auto *t = l->r;

	n->l = t;
	if (t)
		t->setP(n);

	l->r = n;
	l->setP( n->getP() );

	fixParentAndChildren_<1,0,1>(l, n);
}

template<class T_Allocator>
void AVLList<T_Allocator>::rotateRL_(Node *node){
	/*
	 *     A           A             a                 c
	 *      \           \             \              /   \
	 *       b   Rb      c             c     La     /     \
	 *      /    ==>    / \    ==>    / \    ==>   a       B
	 *     c           E*  b         e*  B          \     /
	 *    / \             /             /            e*  D*
	 *   E*  d*          d*            D*
	 */

	// no performance benefit if manually inlined

	rotateR_(node->r);
	rotateL_(node);
}

template<class T_Allocator>
void AVLList<T_Allocator>::rotateLR_(Node *node){
	/*
	 *     A               A             a             c
	 *    /               /             /            /   \
	 *   b       Lb      c             c     Ra     /     \
	 *    \      ==>    / \    ==>    / \    ==>   B       a
	 *     c           b   E*        B   e*         \     /
	 *    / \           \             \              D*  e*
	 *   d*  E*          d*            D*
	 */

	// no performance benefit if manually inlined

	rotateL_(node->l);
	rotateR_(node);
}

template<class T_Allocator>
void AVLList<T_Allocator>::swapLinksRelative_(Node *a, Node *b){
	// a is parent of b

	/*
	 *     p*           p*
	 *      \            \
	 *       a            b
	 *      / \   ==>    / \
	 *     b   c*       a   c*
	 *    / \          / \
	 *   d*  e*       d*  e*
	 */


	assert(a->l == b || a->r == b);
	assert(b->getP() == a);

	using std::swap;

//	swap(a->balance	, b->balance	);
	swap(a->l	, b->l		);
	swap(a->r	, b->r		);
	swap(a->p_	, b->p_		);

	fixParentAndChildren_<1,0,0>(b, a);
	fixParentAndChildren_<0,1,1>(a);

	a->setP(b);

	if (b->l == b){
		b->l = a;
		fixParentAndChildren_<0,0,1>(b);
	}else{ // b->r == b
		fixParentAndChildren_<0,1,0>(b);
		b->r = a;
	}
}

template<class T_Allocator>
void AVLList<T_Allocator>::swapLinksNormal_(Node *a, Node *b){
	using std::swap;

//	swap(a->balance	, b->balance	);
	swap(a->l	, b->l		);
	swap(a->r	, b->r		);
	swap(a->p_	, b->p_		);

	fixParentAndChildren_<1,1,1>(a, b);
	fixParentAndChildren_<1,1,1>(b, a);
}

template<class T_Allocator>
void AVLList<T_Allocator>::swapLinks_(Node *a, Node *b){
	assert(a);
	assert(b);

	if (b->getP() == a)
		return swapLinksRelative_(a, b);

	if (a->getP() == b)
		return swapLinksRelative_(b, a);

	return swapLinksNormal_(a, b);
}

template<class T_Allocator>
void AVLList<T_Allocator>::copyLinks_(Node *a, Node *b){
	assert(a);
	assert(b);

	// std::move shows intention

//	a->balance	= std::move(b->balance	);
	a->l		= std::move(b->l	);
	a->r		= std::move(b->r	);
	a->p_		= std::move(b->p_	);

	fixParentAndChildren_<1,1,1>(a, b);

	// clear other, because if ArenaAllocator is used,
	// pointers may still be valid
	if constexpr(1){
		b->l		= nullptr;
		b->r		= nullptr;
		b->p_		= nullptr;
	}
}

template<class T_Allocator>
template<bool FixP, bool FixL, bool FixR>
void AVLList<T_Allocator>::fixParentAndChildren_(Node *node, const Node *originalParent){
	assert(node);

	if constexpr(FixP){
		assert(originalParent);

		if (auto *parent = node->getP(); !parent){
			// update root_

			root_ = node;
		}else{
			if (parent->l == originalParent)
				parent->l = node;
			else
				parent->r = node;
		}
	}

	if constexpr(FixL){
		if (auto *child = node->l; child)
			child->setP(node);
	}

	if constexpr(FixR){
		if (auto *child = node->r; child)
			child->setP(node);
	}
};

template<class T_Allocator>
void AVLList<T_Allocator>::rebalanceAfterInsert_(Node *node){
	assert(node);

	while(node->getBalance()){
		if (node->getBalance() == +2){
			// right heavy
			if (node->r->getBalance() == +1){
				node->setBalance(0);
				node->r->setBalance(0);

				rotateL_(node);
			}else{ // node->r->balance == -1
				auto const rlBalance = node->r->l->getBalance();

				node->r->l->setBalance(0);
				node->r->setBalance(0);
				node->setBalance(0);

				if (rlBalance == +1)
					node->setBalance(-1);
				else if (rlBalance == -1)
					node->r->setBalance(+1);

				rotateRL_(node);
			}

			break;
		}

		if (node->getBalance() == -2){
			// left heavy
			if (node->l->getBalance() == -1){
				node->setBalance(0);
				node->l->setBalance(0);

				rotateR_(node);
			}else{ // node->r->balance == +1
				auto const lrBalance = node->l->r->getBalance();

				node->l->r->setBalance(0);
				node->l->setBalance(0);
				node->setBalance(0);

				if (lrBalance == -1)
					node->setBalance(+1);
				else if (lrBalance == +1)
					node->l->setBalance(-1);

				rotateLR_(node);
			}

			break;
		}

		auto *parent = node->getP();

		if (!parent)
			return;

		if (parent->l == node)
			parent->decBalance();
		else
			parent->incBalance();

		node = node->getP();
	}
}

template<class T_Allocator>
void AVLList<T_Allocator>::rebalanceAfterErase_(Node *node){
	assert(node);

	while(true){
		if (node->getBalance() == +2){
			// right heavy

			if (node->r->getBalance() == +1){
				node->setBalance(0);
				node->r->setBalance(0);

				rotateL_(node);
			}else if(node->r->getBalance() == 0){
				node->setBalance(+1);
				node->r->setBalance(-1);

				rotateL_(node);

				return;
			}else{ // node->r->balance == -1
				auto const rlBalance = node->r->l->getBalance();

				node->r->l->setBalance(0);
				node->r->setBalance(0);
				node->setBalance(0);

				if (rlBalance == +1)
					node->setBalance(-1);
				else if (rlBalance == -1)
					node->r->setBalance(+1);

				rotateRL_(node);
			}

			node = node->getP();
		}else
		if (node->getBalance() == -2){
			// left heavy

			if (node->l->getBalance() == -1){
				node->setBalance(0);
				node->l->setBalance(0);

				rotateR_(node);
			}else if(node->l->getBalance() == 0){
				node->setBalance(-1);
				node->l->setBalance(+1);

				rotateR_(node);

				return;
			}else{ // node->l->balance == +1
				auto const lrBalance = node->l->r->getBalance();

				node->l->r->setBalance(0);
				node->l->setBalance(0);
				node->setBalance(0);

				if (lrBalance == -1)
					node->setBalance(+1);
				else if (lrBalance == +1)
					node->l->setBalance(-1);

				rotateLR_(node);
			}

			node = node->getP();
		}

		auto *parent = node->getP();

		if (!parent)
			return;

		if (node == parent->l){
			parent->incBalance();

			if (parent->getBalance() == +1)
				return;
		}else{ // node == parent->r
			parent->decBalance();

			if (parent->getBalance() == -1)
				return;
		}

		node = node->getP();
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

		assert(node->getP() == parent);

		assert(node->getBalance() >= -1 && node->getBalance() <= +1);

		if constexpr(CheckHeight){
			auto const balance = testALVTreeIntegrity_height(node->r) - testALVTreeIntegrity_height(node->l);

			if (balance != node->getBalance()){
				printf("here %d\n", node->getBalance());
			}

			assert(balance >= -1 && balance <= +1);
			assert(balance == node->getBalance());
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
void AVLList<T_Allocator>::testIntegrity(std::false_type) const{
	return avl_impl_::testALVTreeIntegrity<0>(root_);
}

template<class T_Allocator>
void AVLList<T_Allocator>::testIntegrity(std::true_type) const{
	return avl_impl_::testALVTreeIntegrity<1>(root_);
}



// ==============================



template<class T_Allocator>
auto AVLList<T_Allocator>::begin() const -> iterator{
	if (root_)
		return {
			avl_impl_::getMinValueNode(root_),
			root_
		};
	else
		return { nullptr, root_ };
}



template<class T_Allocator>
auto AVLList<T_Allocator>::iterator::operator++() -> iterator &{
	node = avl_impl_::getSuccessorNode(node);

	return *this;
}



template<class T_Allocator>
auto AVLList<T_Allocator>::iterator::operator--() -> iterator &{
	if (node == nullptr){
		// standard say, you can not call --it,
		// without check it != begin

		node = avl_impl_::getMaxValueNode(root);

		return *this;
	}

	node = avl_impl_::getPredecessorNode(node);

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

template auto AVLList<MyAllocator::PMAllocator>			::insertF(PairFactory::Normal		&factory) -> InsertResult;
template auto AVLList<MyAllocator::STDAllocator>		::insertF(PairFactory::Normal		&factory) -> InsertResult;
template auto AVLList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Normal		&factory) -> InsertResult;
template auto AVLList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Normal		&factory) -> InsertResult;

template auto AVLList<MyAllocator::PMAllocator>			::insertF(PairFactory::Expires		&factory) -> InsertResult;
template auto AVLList<MyAllocator::STDAllocator>		::insertF(PairFactory::Expires		&factory) -> InsertResult;
template auto AVLList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Expires		&factory) -> InsertResult;
template auto AVLList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Expires		&factory) -> InsertResult;

template auto AVLList<MyAllocator::PMAllocator>			::insertF(PairFactory::Tombstone	&factory) -> InsertResult;
template auto AVLList<MyAllocator::STDAllocator>		::insertF(PairFactory::Tombstone	&factory) -> InsertResult;
template auto AVLList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Tombstone	&factory) -> InsertResult;
template auto AVLList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Tombstone	&factory) -> InsertResult;

template auto AVLList<MyAllocator::PMAllocator>			::insertF(PairFactory::Clone		&factory) -> InsertResult;
template auto AVLList<MyAllocator::STDAllocator>		::insertF(PairFactory::Clone		&factory) -> InsertResult;
template auto AVLList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::Clone		&factory) -> InsertResult;
template auto AVLList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::Clone		&factory) -> InsertResult;

template auto AVLList<MyAllocator::PMAllocator>			::insertF(PairFactory::IFactory		&factory) -> InsertResult;
template auto AVLList<MyAllocator::STDAllocator>		::insertF(PairFactory::IFactory		&factory) -> InsertResult;
template auto AVLList<MyAllocator::ArenaAllocator>		::insertF(PairFactory::IFactory		&factory) -> InsertResult;
template auto AVLList<MyAllocator::SimulatedArenaAllocator>	::insertF(PairFactory::IFactory		&factory) -> InsertResult;

} // namespace hm4

