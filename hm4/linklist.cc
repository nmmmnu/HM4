#include "linklist.h"

#include <cassert>

namespace hm4{


struct LinkList::Node{
	OPair	data;
	Node	*next = nullptr;

public:
	Node(OPair &&data) : data(std::move(data)){}
};

struct LinkList::NodeLocator{
	Node **prev;
	Node *node;
};


// ==============================

LinkList::LinkList(){
	clear_();
}

LinkList::LinkList(LinkList &&other):
		head_		(std::move(other.head_	)),
		lc_		(std::move(other.lc_	)){
	other.clear_();
}

bool LinkList::clear(){
	for(const Node *node = head_; node; ){
		const Node *copy = node;

		node = node->next;

		delete copy;
	}

	clear_();

	return true;
}

bool LinkList::insert(OPair&& newdata){
	assert(newdata);

	std::string_view const key = newdata->getKey();

	const auto loc = locate_(key);

	if (loc.node){
		OPair &olddata = loc.node->data;

		// handle overwrite,
		// keep the node, overwrite the data

		// check if the data in database is valid
		if (! newdata->isValid(*olddata)){
			// newdata will be magically destroyed.
			return false;
		}

		lc_.upd( olddata->bytes(), newdata->bytes() );

		// copy assignment
		olddata = std::move(newdata);

		return true;
	}

	size_t const size = newdata->bytes();

	Node *newnode = new(std::nothrow) Node(std::move(newdata));
	if (newnode == nullptr){
		// newdata will be magically destroyed.
		return false;
	}

	newnode->next = *loc.prev;
	*loc.prev = newnode;

	lc_.inc(size);

	return true;
}

#if 0
const Pair *LinkList::operator[](std::string_view const key) const{
	assert(!key.empty());

	const Node *node = locateNode_(key, true);

	return node ? node->data.get() : nullptr;
}
#endif

bool LinkList::erase(std::string_view const key){
	assert(!key.empty());

	const auto loc = locate_(key);

	if (loc.node){
		*loc.prev = loc.node->next;

		lc_.dec( loc.node->data->bytes() );

		delete loc.node;
	}

	return true;
}

// ==============================

void LinkList::clear_(){
	lc_.clr();

	head_ = nullptr;
}

auto LinkList::locate_(std::string_view const key) -> NodeLocator{
	assert(!key.empty());

	Node **jtable = & head_;

	for(Node *node = *jtable; node; node = node->next){
		const OPair & data = node->data;

		int const cmp = data.cmp(key);

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
		const OPair & data = node->data;

		int const cmp = data.cmp(key);

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


