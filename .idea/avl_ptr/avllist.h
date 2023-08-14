#ifndef AVL_LIST_H_
#define AVL_LIST_H_

#include "ilist.h"
#include "listcounter.h"



namespace hm4{

template<class T_Allocator>
class AVLList{
public:
	using Allocator		= T_Allocator;
	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

public:
	class iterator;

public:
	AVLList(Allocator &allocator) : allocator_(& allocator){}

	AVLList(AVLList &&other) :
				lc_		(std::move(other.lc_		)),
				allocator_	(std::move(other.allocator_	)),
				root_		(std::move(other.root_		)){
		other.clear_<false>();
	}

	~AVLList(){
		deallocateTree_(root_);
	}

private:
	class Node;

	ListCounter	lc_;
	Allocator	*allocator_;
	Node		*root_		= nullptr;

public:
	bool clear(){
		clear_<true>();

		return true;
	}

	template<class PFactory>
	iterator insertF(PFactory &factory);

	bool erase_(std::string_view const key);

	auto size() const{
		return lc_.size();
	}

	auto mutable_size() const{
		return size();
	}

	void mutable_notify(const Pair *, PairFactoryMutableNotifyMessage const &msg){
		lc_.upd(msg.bytes_old, msg.bytes_new);
	}

	auto bytes() const{
		return lc_.bytes();
	}

	auto mutable_bytes() const{
		return bytes();
	}

	constexpr static void crontab(){
	}

	const Allocator &getAllocator() const{
		return *allocator_;
	}

	Allocator &getAllocator(){
		return *allocator_;
	}

public:
	template<bool B>
	iterator find(std::string_view const key, std::bool_constant<B> exact) const;

	iterator begin() const;

	constexpr static iterator end();

public:
	void testALVTreeIntegrity(std::false_type) const;
	void testALVTreeIntegrity(std::true_type) const;
	size_t height() const;

private:
	template<bool DeallocateTree>
	void clear_(){
		if constexpr(DeallocateTree)
			deallocateTree_(root_);

		lc_.clr();
		root_ = nullptr;
	}

	void deallocate_(Node *node);
	void deallocateTree_(Node *node);

private:
	void rotateL_(Node *n);
	void rotateR_(Node *n);

	void rotateRL_(Node *node);
	void rotateLR_(Node *node);

	void rebalanceAfterInsert_(Node *node);
	void rebalanceAfterErase_(Node *node);
};



// ==============================



template<class T_Allocator>
class AVLList<T_Allocator>::iterator{
public:
	constexpr iterator(const Node *node) : node(node){}

public:
	using difference_type	= std::ptrdiff_t;
	using value_type	= const Pair;
	using pointer		= value_type *;
	using reference		= value_type &;
	using iterator_category	= std::forward_iterator_tag;
	// avl tree can support bi-directiona iterator as well

public:
	iterator &operator++();

	reference operator*() const;

	bool operator==(const iterator &other) const{
		return node == other.node;
	}

	bool operator!=(const iterator &other) const{
		return ! operator==(other);
	}

	pointer operator ->() const{
		return & operator*();
	}

private:
	const Node *node;
};



// ==============================



template<class T_Allocator>
constexpr auto AVLList<T_Allocator>::end() -> iterator{
	return nullptr;
}



} // namespace hm4

#endif

