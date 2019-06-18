#ifndef HIDDEN_POINTER_ITERATOR_H_CC_
#define HIDDEN_POINTER_ITERATOR_H_CC_

#include "hidden_pointer_iterator.h"

template<class hidden_t, class T>
bool hidden_pointer_iterator<hidden_t, T>::operator ==(hidden_pointer_iterator const &other) const{
	return pos == other.pos;
}

template<class hidden_t, class T>
auto hidden_pointer_iterator<hidden_t, T>::operator ++() -> hidden_pointer_iterator &{
	++pos;
	return *this;
}

#endif

