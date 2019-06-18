#ifndef HIDDEN_POINTER_ITERATOR_H_
#define HIDDEN_POINTER_ITERATOR_H_

template<class hidden_t, class T>
class hidden_pointer_iterator{
public:
	using difference_type = ptrdiff_t;
	using value_type = T;
	using pointer = value_type *;
	using reference = value_type &;
	using iterator_category = std::forward_iterator_tag;

public:
	hidden_pointer_iterator(const hidden_t *pos) : pos(pos){}

	bool operator !=(hidden_pointer_iterator const &other) const{
		return ! ( *this == other);
	}

	bool operator ==(hidden_pointer_iterator const &other) const;
	hidden_pointer_iterator &operator ++();
	T operator *() const;

private:
	const hidden_t *pos;
};

#endif

