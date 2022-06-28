#include "linklist.h"

#include <cassert>

#include "hpair.h"

namespace hm4{

struct LinkList::Node{
	HPair::HKey	hkey;
	Pair		*data;
	Node		*next = nullptr;

	int cmp(HPair::HKey const hkey, std::string_view const key) const{
		return HPair::cmp(this->hkey, *this->data, hkey, key);
	}
};

struct LinkList::NodeLocator{
	Node **prev;
	Node *node;
};

namespace{
	constexpr bool corruptionCheck = true;

	[[maybe_unused]]
	void corruptionExit(){
		fprintf(stderr, "====================================\n");
		fprintf(stderr, "=== Detected LinkList corruption ===\n");
		fprintf(stderr, "====================================\n");
		exit(100);
	}
}

// ==============================

LinkList::LinkList(Allocator &allocator) : allocator_(& allocator){
	clear_();
}

LinkList::LinkList(LinkList &&other):
			head_		(std::move(other.head_		)),
			lc_		(std::move(other.lc_		)),
			allocator_	(std::move(other.allocator_	)){
	other.clear_();
}

void LinkList::deallocate_(Node *node){
	using namespace MyAllocator;

	deallocate(allocator_, node->data);
	deallocate(allocator_, node);
}

void LinkList::clear_(){
	lc_.clr();

	head_ = nullptr;
}

bool LinkList::clear(){
	if (allocator_->reset() == false){
		for(Node *node = head_; node; ){
			Node *copy = node;

			node = node->next;

			deallocate_(copy);
		}
	}

	clear_();

	return true;
}

auto LinkList::insertSmartPtrPair_(MyAllocator::SmartPtrType<Pair, Allocator> &&newdata) -> iterator{
	if (!newdata)
		return this->end();

	auto const &key = newdata->getKey();

	const auto loc = locate_(key);

	if (loc.node){
		// update in place.

		Pair *olddata = loc.node->data;

		// check if the data in database is valid
		if (! newdata->isValidForReplace(*olddata) ){
			// newdata will be magically destroyed.
			return this->end();
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

bool LinkList::erase(std::string_view const key){
	// better Pair::check(key), but might fail because of the caller.
	assert(!key.empty());

	auto loc = locate_(key);

	if (loc.node){
		if constexpr(corruptionCheck)
			if (*loc.prev != loc.node)
				corruptionExit();

		*loc.prev = loc.node->next;

		lc_.dec( loc.node->data->bytes() );

		deallocate_(loc.node);
	}

	return true;
}

// ==============================

auto LinkList::locate_(std::string_view const key) -> NodeLocator{
	// better Pair::check(key), but might fail because of the caller.
	assert(!key.empty());

	Node **jtable = & head_;

	auto hkey = HPair::SS::create(key);

	for(Node *node = *jtable; node; node = node->next){
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

auto LinkList::locateNode_(std::string_view const key, bool const exact) const -> const Node *{
	assert(!key.empty());

	auto hkey = HPair::SS::create(key);

	for(const Node *node = head_; node; node = node->next){
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


auto LinkList::iterator::operator++() -> iterator &{
	node_ = node_->next;

	return *this;
}

const Pair &LinkList::iterator::operator*() const{
	assert(node_);

	return *(node_->data);
}


} // namespace


