#ifndef HIDDEN_POINTER_ITERATOR_H_
#define HIDDEN_POINTER_ITERATOR_H_

#if 0
namespace hpi{
	bool eq(const hidden_t *a, const hidden_t *b)	__attribute__((const))	;
	void inc(const hidden_t * &a)						;
	T conv(const hidden_t *a)			__attribute__((pure))	;
}
#endif

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
		return ! hpi::eq( pos, other.pos);
	}

	bool operator ==(hidden_pointer_iterator const &other) const{
		return hpi::eq( pos, other.pos);
	}

	hidden_pointer_iterator &operator ++(){
		hpi::inc(pos);
		return *this;
	}

	T operator *() const{
		return hpi::conv(pos);
	}

private:
	const hidden_t *pos;
};

#endif

