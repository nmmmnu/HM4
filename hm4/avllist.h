#ifndef AVL_LIST_H_
#define AVL_LIST_H_

#include "ilist.h"
#include "listcounter.h"

#include <iterator>



namespace hm4{

template<class T_Allocator>
class AVLList{
public:
	using Allocator		= T_Allocator;
	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

public:
	class iterator;
	using reverse_iterator	= std::reverse_iterator<iterator>;

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
	struct Node;

	ListCounter	lc_;
	Allocator	*allocator_;
	Node		*root_		= nullptr;

public:
	static size_t const INTERNAL_NODE_SIZE;

public:
	constexpr static std::string_view getName(){
		return "AVLList";
	}

public:
	bool clear(){
		clear_<true>();

		return true;
	}

	template<class PFactory>
	InsertResult insertF(PFactory &factory);

	InsertResult erase_(std::string_view const key);

	auto size() const{
		return lc_.size();
	}

	auto empty() const{
		return size() == 0;
	}

	auto const &mutable_list() const{
		return *this;
	}

	void mutable_notify(PairFactoryMutableNotifyMessage const &msg){
		lc_.upd(msg.bytes_old, msg.bytes_new);
	}

	auto bytes() const{
		return lc_.bytes();
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
	iterator    find   (std::string_view const key) const;
	const Pair *getPair(std::string_view const key) const;

	iterator begin() const;

	constexpr iterator end() const;

public:
	template<bool B>
	reverse_iterator rfind(std::string_view const key, std::bool_constant<B> exact) const{
		if (auto it = find(key, exact); it != end())
			return std::make_reverse_iterator(++it);
		else
			return rend();
	}

	reverse_iterator rbegin() const noexcept{
		return std::make_reverse_iterator( end() );
	}

	reverse_iterator rend() const noexcept{
		return std::make_reverse_iterator( begin() );
	}

public:
	void testIntegrity(std::false_type) const;
	void testIntegrity(std::true_type) const;
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

private:
	void swapLinks_(Node *a, Node *b);
	void swapLinksRelative_(Node *a, Node *b);
	void swapLinksNormal_(Node *a, Node *b);

	void copyLinks_(Node *a, Node *b);

	template<bool FixP, bool FixL, bool FixR>
	void fixParentAndChildren_(Node *node, const Node *original = nullptr);
};



// ==============================



template<class T_Allocator>
class AVLList<T_Allocator>::iterator{
public:
	constexpr iterator(const Node *node, const Node *root) : node(node), root(root){}

public:
	using difference_type	= std::ptrdiff_t;
	using value_type	= const Pair;
	using pointer		= value_type *;
	using reference		= value_type &;
	using iterator_category	= std::bidirectional_iterator_tag;

public:
	iterator &operator++();
	iterator &operator--();

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
	const Node *root;
};



// ==============================



template<class T_Allocator>
constexpr auto AVLList<T_Allocator>::end() const -> iterator{
	return { nullptr, root_ };
}



} // namespace hm4

#endif

