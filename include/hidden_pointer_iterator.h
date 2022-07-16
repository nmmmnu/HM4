#ifndef HIDDEN_POINTER_ITERATOR_H_
#define HIDDEN_POINTER_ITERATOR_H_

#include <iterator>

#if 0
struct hpi{
	using value_type	= int;
	using convert_type	= int;

	static bool eq(const value_type *a, const value_type *b)	__attribute__((const)){
		return a == b;
	}
	static void inc(const value_type * &a);
	static convert_type conv(const value_type *a)			__attribute__((pure));
};
#endif

template<class HPI>
class hidden_pointer_iterator{
public:
	using difference_type = std::ptrdiff_t;
	using value_type = typename HPI::value_type;
	using pointer = value_type *;
	using reference = value_type &;
	using iterator_category = std::forward_iterator_tag;

public:
	hidden_pointer_iterator(const value_type *pos) : pos(pos){}

	bool operator !=(hidden_pointer_iterator const &other) const{
		return ! HPI::eq( pos, other.pos);
	}

	bool operator ==(hidden_pointer_iterator const &other) const{
		return HPI::eq( pos, other.pos);
	}

	hidden_pointer_iterator &operator ++(){
		HPI::inc(pos);
		return *this;
	}

	auto operator *() const{
		return HPI::conv(pos);
	}

private:
	const value_type *pos;
};

#endif

