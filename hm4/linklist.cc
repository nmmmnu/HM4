#include "linklist.h"

#include <cassert>

namespace hm4{


struct LinkList::Node{
	Pair	*data;
	Node	*next = nullptr;
};

struct LinkList::NodeLocator{
	Node **prev;
	Node *node;
};

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
	allocator_->deallocate(node->data);
	allocator_->deallocate(node);
}

void LinkList::clear_(){
	lc_.clr();

	head_ = nullptr;
}

bool LinkList::clear(){
	if (allocator_->need_deallocate() ){
		for(Node *node = head_; node; ){
			Node *copy = node;

			node = node->next;

			deallocate_(copy);
		}
	}

	clear_();

	return true;
}

bool LinkList::insert(
		std::string_view const key, std::string_view const val,
		uint32_t const expires, uint32_t const created){

	auto newdata = Pair::up::create(*allocator_, key, val, expires, created);

	if (!newdata)
		return false;

	const auto loc = locate_(key);

	if (loc.node){
		// update in place.

		Pair *olddata = loc.node->data;

		// check if the data in database is valid
		if (! newdata->isValid(*olddata)){
			// newdata will be magically destroyed.
			return false;
		}

		lc_.upd( olddata->bytes(), newdata->bytes() );

		// assign new pair
		loc.node->data = newdata.release();

		// deallocate old pair
		allocator_->deallocate(olddata);

		return true;
	}

	// create new node

	size_t const size = newdata->bytes();

	Node *newnode = static_cast<Node *>(allocator_->allocate(sizeof(Node)));
	if (newnode == nullptr){
		// newdata will be magically destroyed.
		return false;
	}

	newnode->data = newdata.release();

	newnode->next = std::exchange(*loc.prev, newnode);

	lc_.inc(size);

	return true;
}

bool LinkList::erase(std::string_view const key){
	assert(Pair::check(key));

	auto loc = locate_(key);

	if (loc.node){
		*loc.prev = loc.node->next;

		lc_.dec( loc.node->data->bytes() );

		deallocate_(loc.node);
	}

	return true;
}

// ==============================

auto LinkList::locate_(std::string_view const key) -> NodeLocator{
	assert(!key.empty());

	Node **jtable = & head_;

	for(Node *node = *jtable; node; node = node->next){
		const Pair *data = node->data;

		int const cmp = data->cmp(key);

		if (cmp >= 0){
			if (cmp == 0){
				return { jtable, node };
			}else{
				return { jtable, nullptr };
			}
		}

		jtable = & node->next;
	}

	return { jtable, nullptr };
}

auto LinkList::locateNode_(std::string_view const key, bool const exact) const -> const Node *{
	assert(!key.empty());

	for(const Node *node = head_; node; node = node->next){
		const Pair *data = node->data;

		int const cmp = data->cmp(key);

		if (cmp >= 0){
			if (cmp == 0){
				return node;
			}else{
				return exact ? nullptr : node;
			}
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


