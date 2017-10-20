#include "linklist.h"

#include <cassert>

namespace hm4{


struct LinkList::Node{
	OPair	data;
	Node	*next = nullptr;

public:
	Node(OPair &&data) : data(std::move(data)){}
};


// ==============================


LinkList::LinkList(){
	clear_();
}

LinkList::LinkList(LinkList &&other):
		head_		(std::move(other.head_		)),
		dataCount_	(std::move(other.dataCount_	)),
		dataSize_	(std::move(other.dataSize_	)){
	other.clear_();
}

bool LinkList::clear(){
	for(Node *node = head_; node; ){
		const Node *copy = node;

		node = node->next;

		delete copy;
	}

	clear_();

	return true;
}

bool LinkList::insert(OPair&& newdata){
	assert(newdata);

	const StringRef &key = newdata->getKey();

	Node *prev = nullptr;
	for(Node *node = head_; node; node = node->next){
		OPair & olddata = node->data;

		int const cmp = olddata->cmp(key);

		if (cmp == 0){
			// handle overwrite,
			// keep the node, overwrite the data

			// check if the data in database is valid
			if (! newdata->isValid(*olddata)){
				// newdata will be magically destroyed.
				return false;
			}

			dataSize_ = dataSize_
					- olddata->bytes()
					+ newdata->bytes();

			// copy assignment
			olddata = std::move(newdata);

			return true;
		}

		if (cmp > 0)
			break;

		prev = node;
	}

	size_t const size = newdata->bytes();

	Node *newnode = new(std::nothrow) Node(std::move(newdata));
	if (newnode == nullptr){
		// newdata will be magically destroyed.
		return false;
	}

	// put new pair here...
	if (prev){
		// we are at the middle
		newnode->next = prev->next;
		prev->next = newnode;
	}else{
		newnode->next = head_;
		head_ = newnode;
	}

	dataSize_ += size;
	dataCount_++;

	return true;
}

const Pair *LinkList::operator[](const StringRef &key) const{
	assert(!key.empty());

	const Node *node = locate_(key, true);

	return node ? node->data.get() : nullptr;
}

bool LinkList::erase(const StringRef &key){
	Node *prev = nullptr;
	for(Node *node = head_; node; node = node->next){
		const OPair & data = node->data;
		int const cmp = data->cmp(key);

		if (cmp == 0){
			if (prev){
				prev->next = node->next;
			}else{
				// First node...
				head_ = node->next;
			}

			dataSize_ -= data->bytes();
			--dataCount_;

			delete node;
			return true;
		}

		if (cmp > 0)
			break;

		prev = node;
	}

	return true;
}

// ==============================

void LinkList::clear_(){
	dataCount_ = 0;
	dataSize_ = 0;
	head_ = nullptr;
}

const LinkList::Node *LinkList::locate_(const StringRef &key, bool const exact) const{
	// precondition
	assert(!key.empty());
	// eo precondition

	for(const Node *node = head_; node; node = node->next){
		const OPair & data = node->data;

		int const cmp = data->cmp(key);

		if (cmp == 0)
			return node;

		if (cmp > 0)
			return exact ? nullptr : node;
	}

	return nullptr;
}


// ==============================


LinkList::Iterator &LinkList::Iterator::operator++(){
	node_ = node_->next;

	return *this;
}

const Pair &LinkList::Iterator::operator*() const{
	assert(node_);

	return *(node_->data);
}


} // namespace


