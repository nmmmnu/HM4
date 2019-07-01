
class DiskList::forward_iterator {
public:
	forward_iterator(DiskList const &list, difference_type const ptr) :
				list(&list),
				ptr(ptr){}

	forward_iterator(DiskList const &list, size_type const ptr) :
				forward_iterator(list, static_cast<difference_type>(ptr)){}

	using iterator = forward_iterator;

public:
	using difference_type = DiskList::difference_type;
	using value_type = const Pair;
	using pointer = value_type *;
	using reference = value_type &;
	using iterator_category = std::forward_iterator_tag;

public:
	iterator &operator++(){
		++ptr;

		if (sorted && pair)
			pair = list->fdGetNext_(pair);

		return *this;
	}

	reference operator*() const{
		if (sorted)
			return *pair;
		else
			return (*list)[ sc__(ptr) ];
	}

public:
	bool operator==(iterator const &other) const{
		return ptr == other.ptr;
	}

	bool operator!=(iterator const &other) const{
		return ! operator==(other);
	}

	pointer operator ->() const{
		return & operator*();
	}

private:
	static size_type sc__(difference_type const a){
		return static_cast<size_type>(a);
	}

	const Pair *initPair_() const{
		if (list->sorted() == false)
			return nullptr;

		if (list->size() <= sc__(ptr))
			return nullptr;

		return list->fdGetAt_( sc__(ptr) );
	}

private:
	const DiskList	*list;
	difference_type	ptr;

	const Pair	*pair	= initPair_();

	bool		sorted	= list->sorted();
};



class DiskList::random_access_iterator{
public:
	random_access_iterator(DiskList const &list, difference_type const ptr) :
				list(&list),
				ptr(ptr){}

	random_access_iterator(DiskList const &list, size_type const ptr) :
				random_access_iterator(list, static_cast<difference_type>(ptr)){}

	explicit
	operator forward_iterator(){
		return { *list, ptr };
	}

	using iterator = random_access_iterator;

public:
	using difference_type	= DiskList::difference_type;
	using value_type	= const Pair;
	using pointer		= value_type *;
	using reference		= value_type &;
	using iterator_category	= std::random_access_iterator_tag;

private:
	using size_type		= DiskList::size_type;

private:
	iterator clone(difference_type const off) const{
		return { *list, off };
	}

	reference getAt(difference_type const off) const{
		return (*list)[ static_cast<size_type>(off) ];
	}

public:
	// increment / decrement
	iterator &operator++(){
		++ptr;
		return *this;
	}

	iterator &operator--(){
		--ptr;
		return *this;
	}

	iterator operator++(int){
		auto tmp = ptr;
		++ptr;
		return clone(tmp);
	}

	iterator operator--(int){
		auto tmp = ptr;
		--ptr;
		return clone(tmp);
	}

public:
	// arithmetic
	// https://www.boost.org/doc/libs/1_50_0/boost/container/vector.hpp

	iterator& operator+=(difference_type const off){
		ptr += off;
		return *this;
	}

	iterator operator +(difference_type const off) const{
		return clone(ptr + off);
	}

	iterator& operator-=(difference_type const off){
		ptr -= off;
		return *this;
	}

	iterator operator -(difference_type const off) const{
		return clone(ptr - off);
	}

	friend iterator operator +(difference_type const  off, iterator const &it){
		return it.clone(it.ptr + off);
	}

	difference_type operator -(iterator const &other) const{
		return ptr - other.ptr;
	}

public:
	// compare
	bool operator==(iterator const &other) const{
		return ptr == other.ptr;
	}

	bool operator!=(iterator const &other) const{
		return ptr != other.ptr;
	}

	bool operator >(iterator const &other) const{
		return ptr >  other.ptr;
	}

	bool operator>=(iterator const &other) const{
		return ptr >= other.ptr;
	}

	bool operator <(iterator const &other) const{
		return ptr <  other.ptr;
	}

	bool operator<=(iterator const &other) const{
		return ptr <= other.ptr;
	}

public:
	// dereference

	reference operator[](difference_type const off) const{
		return getAt(ptr + off);
	}

	reference operator*() const{
		return getAt(ptr);
	}

	pointer operator ->() const{
		return & operator*();
	}

private:
	const DiskList	*list;
	difference_type	ptr;
};

