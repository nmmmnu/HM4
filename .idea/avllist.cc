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

	constexpr void clearDebug(){
		if constexpr(0)
			clear(nullptr);
	}

	int cmp(std::string_view const key) const{
		return data.cmp(key);
	}
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
		Node *parent = node->p;

		while(parent && node == parent->l){
			node = parent;
			parent = parent->p;
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
		Node *parent = node->p;

		while(parent && node == parent->r){
			node = parent;
			parent = parent->p;
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

		if (node->balance < 0)
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

	bool const b = (key == "a~106");

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
		if (b){
			printf("BBB : CASE3\n");
			testALVTreeIntegrity(std::true_type{});
		}

		// CASE 3 - node two children
		auto *successor = avl_impl_::getMinValueNode(node->r);

		// I know successor left child is nullptr
		// but erase is slow and not really used anyway
		swapLinks_(node, successor);

		if (b){
			printf("BBB : CASE3 test\n");
			testALVTreeIntegrity(std::true_type{});
			printf("BBB : CASE3 test done\n");
		}

		// Silently proceed to CASE 2
		node = successor;
	}

	if (b)
		testALVTreeIntegrity(std::true_type{});

	if (auto *child = node->l ? node->l : node->r; child){
		if (b) printf("BBB : CASE2\n");

		// CASE 2: node with only one child
		child->p = node->p;

		// do not use fixParentAndChildren_(), because it does more work.

		if (!node->p){
			lc_.dec(node->data.bytes());
			deallocate_(node);

			this->root_ = child;

			return InsertResult::deleted();
		}

		if (auto *parent = node->p; node == parent->l){
			parent->l = child;
			++parent->balance;

			lc_.dec(node->data.bytes());
			deallocate_(node);

			if (parent->balance == +1){
				return InsertResult::deleted();
			}else{
				rebalanceAfterErase_(parent);
				return InsertResult::deleted();
			}
		}else{ // node == parent->r
			parent->r = child;
			--parent->balance;

			lc_.dec(node->data.bytes());
			deallocate_(node);

			if (parent->balance == -1){
				return InsertResult::deleted();
			}else{
				rebalanceAfterErase_(parent);
				return InsertResult::deleted();
			}
		}
	}

	// CASE 1: node with no children

	if (b) printf("BBB : CASE1\n");

	if (!node->p){
		if (b) printf("BBB : CASE1 NO CHILDREN\n");

		lc_.dec(node->data.bytes());
		deallocate_(node);

		this->root_ = nullptr;

		return InsertResult::deleted();
	}

	if (auto *parent = node->p; node == parent->l){
		if (b) printf("BBB : CASE1 L\n");

		parent->l = nullptr;
		++parent->balance;

		lc_.dec(node->data.bytes());
		deallocate_(node);

		if (parent->balance == +1){
			return InsertResult::deleted();
		}else{
			rebalanceAfterErase_(parent);
			return InsertResult::deleted();
		}
	}else{ // node == parent->r
		if (b) printf("BBB : CASE1 R\n");

		parent->r = nullptr;
		--parent->balance;

		lc_.dec(node->data.bytes());
		deallocate_(node);

		if (parent->balance == -1){
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
				--node->balance;
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
				++node->balance;
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
template<bool ExactEvaluation>
auto AVLList<T_Allocator>::find(std::string_view const key, std::bool_constant<ExactEvaluation>) const -> iterator{
	assert(!key.empty());

	auto *node = root_;

	while(node){
		int const cmp = node->cmp(key);

		if (cmp > 0){
			if constexpr(!ExactEvaluation)
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
			if constexpr(!ExactEvaluation)
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
void AVLList<T_Allocator>::deallocate_(Node *node){
	node->clearDebug();

	using namespace MyAllocator;
	deallocate(allocator_, node);
}

template<class T_Allocator>
void AVLList<T_Allocator>::deallocateTree_(Node *node){
	if (allocator_->reset() == false){

		// Morris traversal
		// https://stackoverflow.com/questions/69777742/how-can-i-delete-a-binary-tree-with-o1-additional-memory

		Node *tail = node;

		while (node){
			// update tail
			while (tail->l)
				tail = tail->l;

			// move right to the end of the "list"
			// needs to happen before retrieving next,
			// since the node may only have a right subtree
			tail->l = node->r;

			Node *temp = node->l;

			deallocate_(node);

			node = temp;
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
		t->p = n;

	r->l = n;
	r->p = n->p;

	fixParentAndChildren_<1,0>(r, n);
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
		t->p = n;

	l->r = n;
	l->p = n->p;

	fixParentAndChildren_<0,1>(l, n);
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
void AVLList<T_Allocator>::swapLinks_(Node *a, Node *b){
	assert(a);
	assert(b);

	auto const parentAndChild = (a->p == b || b->p == a);

	using std::swap;

	swap(a->balance	, b->balance	);
	swap(a->l	, b->l		);
	swap(a->r	, b->r		);
	swap(a->p	, b->p		);

	// normal case
	if (!parentAndChild){
		fixParentAndChildren_<1,1>(a, b);
		fixParentAndChildren_<1,1>(b, a);

		return;
	}

	// special case
	// [a] -> [b]
	// or
	// [b] -> [a]

	if (a->p == a){
		// [a] was parent of [b]
		// [b] is  parent of [a] now.

		a->p = b;

		if (b->l == b){
			fixParentAndChildren_<0,1>(b, a);
			b->l = a;
		}else{
			fixParentAndChildren_<1,0>(b, a);
			b->r = a;
		}
	}else{ // b->p == b
		// [b] was parent of [a]
		// [a] is  parent of [b] now.

		b->p = a;

		if (a->l == a){
			fixParentAndChildren_<0,1>(a, b);
			a->l = b;
		}else{
			fixParentAndChildren_<1,0>(a, b);
			a->r = b;
		}
	}
}

template<class T_Allocator>
void AVLList<T_Allocator>::copyLinks_(Node *a, Node *b){
	assert(a);
	assert(b);

	// std::move shows intention

	a->balance	= std::move(b->balance	);
	a->l		= std::move(b->l	);
	a->r		= std::move(b->r	);
	a->p		= std::move(b->p	);

	fixParentAndChildren_<1,1>(a, b);

	// clear other, because if ArenaAllocator is used,
	// pointers may still be valid
	if constexpr(1){
		b->l		= nullptr;
		b->r		= nullptr;
		b->p		= nullptr;
	}
}

template<class T_Allocator>
template<bool FixL, bool FixR>
void AVLList<T_Allocator>::fixParentAndChildren_(Node *node, const Node *original){
	assert(node);
	assert(original);

	if (auto *parent = node->p; !parent){
		// update root_

		root_ = node;
	}else{
		if (parent->l != original && parent->r != original){
			printf("%p %p %p\n", (void *)original, (void *)parent->l, (void *)parent->r);
		}

		if (parent->l == original)
			parent->l = node;
		else
			parent->r = node;
	}

	if constexpr(FixL){
		if (auto *child = node->l; child){
			if (child->p != original)
				printf("%p %p\n", (void *)child->p, (void *)original);

			child->p = node;
		}
	}

	if constexpr(FixR){
		if (auto *child = node->r; child){
			if (child->p != original)
				printf("%p %p\n", (void *)child->p, (void *)original);

			child->p = node;
		}
	}
};

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
		if (node->l == nullptr && node->r == nullptr && node->p == nullptr){
			printf("BBB we are here\n");
			node->data.print();
		}

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

			return;
		}else
		if (node->balance == -2){
			// left heavy

			if (node->l->balance == -1){
				node->balance = 0;
				node->l->balance = 0;

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
					node->balance = +1;
				else if (lrBalance == +1)
					node->l->balance = -1;

				rotateLR_(node);
			}

			node = node->p;

			return;
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

template auto AVLList<MyAllocator::PMAllocator>			::find(std::string_view const key, std::true_type ) const -> iterator;
template auto AVLList<MyAllocator::STDAllocator>		::find(std::string_view const key, std::true_type ) const -> iterator;
template auto AVLList<MyAllocator::ArenaAllocator>		::find(std::string_view const key, std::true_type ) const -> iterator;
template auto AVLList<MyAllocator::SimulatedArenaAllocator>	::find(std::string_view const key, std::true_type ) const -> iterator;

template auto AVLList<MyAllocator::PMAllocator>			::find(std::string_view const key, std::false_type) const -> iterator;
template auto AVLList<MyAllocator::STDAllocator>		::find(std::string_view const key, std::false_type) const -> iterator;
template auto AVLList<MyAllocator::ArenaAllocator>		::find(std::string_view const key, std::false_type) const -> iterator;
template auto AVLList<MyAllocator::SimulatedArenaAllocator>	::find(std::string_view const key, std::false_type) const -> iterator;

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

