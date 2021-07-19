#include "intrusivelinklist.h"

#include <cassert>

#include "hpair.h"

namespace hm4{

struct IntrusiveLinkList::Node{
	Node		*next = nullptr;
	HPair::HKey	hkey;
	Pair		data;

	const Pair &getPair() const{
		return data;
	}

	Pair &getPair(){
		return data;
	}
};

struct IntrusiveLinkList::NodeLocator{
	Node **prev;
	Node *node;
};

// ==============================

IntrusiveLinkList::IntrusiveLinkList(Allocator &allocator) : allocator_(& allocator){
	clear_();
}

IntrusiveLinkList::IntrusiveLinkList(IntrusiveLinkList &&other):
			head_		(std::move(other.head_		)),
			lc_		(std::move(other.lc_		)),
			allocator_	(std::move(other.allocator_	)){
	other.clear_();
}

void IntrusiveLinkList::clear_(){
	lc_.clr();

	head_ = nullptr;
}

bool IntrusiveLinkList::clear(){
	if (allocator_->reset() == false){
		for(Node *node = head_; node; ){
			Node *copy = node;

			node = node->next;

			allocator_->deallocate(copy);
		}
	}

	clear_();

	return true;
}

template<class Bytes, class Create>
auto IntrusiveLinkList::insert_(std::string_view const key, Bytes bytes, Create create) -> iterator{
	size_t const nodeSize = sizeof(Node) - sizeof(Pair) + bytes();

	Node *newnode = static_cast<Node *>(allocator_->allocate(nodeSize));

	if (newnode == nullptr)
		return this->end();

	create(& newnode->getPair());

	newnode->hkey = HPair::SS::create(key);

	const auto loc = locate_(key);

	if (loc.node){
		// check if the data in database is valid
		if (! newnode->getPair().isValidForReplace(loc.node->getPair()) ){
			allocator_->deallocate(newnode);
			return this->end();
		}

		eraseNode_(loc);

		// fallthrought
	}

	// create new node

	newnode->next = std::exchange(*loc.prev, newnode);

	lc_.inc(newnode->getPair().bytes());

	return { newnode };
}

auto IntrusiveLinkList::insert(	std::string_view const key, std::string_view const val,
				uint32_t const expires, uint32_t const created) -> iterator{

	if (Pair::check(key, val) == false)
		return this->end();

	return insert_(
		key,
		[&](){
			return Pair::bytes(key, val);
		},
		[&](Pair *mem){
			Pair::createInRawMemory(mem, key, val, expires, created);
		}
	);
}

auto IntrusiveLinkList::insert(Pair const &src) -> iterator{
	return insert_(
		src.getKey(),
		[&](){
			return src.bytes();
		},
		[&](Pair *mem){
			Pair::cloneInRawMemory(mem, src);
		}
	);
}

void IntrusiveLinkList::eraseNode_(NodeLocator const &loc){
	*loc.prev = loc.node->next;

	lc_.dec( loc.node->getPair().bytes() );

	allocator_->deallocate(loc.node);
}

bool IntrusiveLinkList::erase(std::string_view const key){
	// better Pair::check(key), but might fail because of the caller.
	assert(!key.empty());

	auto const loc = locate_(key);

	if (loc.node){
		eraseNode_(loc);
		return true;
	}

	return false;
}

// ==============================

auto IntrusiveLinkList::locate_(std::string_view const key) -> NodeLocator{
	// better Pair::check(key), but might fail because of the caller.
	assert(!key.empty());

	Node **jtable = & head_;

	auto hkey = HPair::SS::create(key);

	for(Node *node = *jtable; node; node = node->next){
		// this allows comparisson with single ">", instead of more complicated 3-way.
		if (node->hkey >= hkey){
			int const cmp = node->getPair().cmp(key);

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

auto IntrusiveLinkList::locateNode_(std::string_view const key, bool const exact) const -> const Node *{
	assert(!key.empty());

	auto hkey = HPair::SS::create(key);

	for(const Node *node = head_; node; node = node->next){
		// this allows comparisson with single ">", instead of more complicated 3-way.
		if (node->hkey >= hkey){
			int const cmp = node->getPair().cmp(key);

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


auto IntrusiveLinkList::iterator::operator++() -> iterator &{
	node_ = node_->next;

	return *this;
}

const Pair &IntrusiveLinkList::iterator::operator*() const{
	assert(node_);

	return node_->getPair();
}


} // namespace


