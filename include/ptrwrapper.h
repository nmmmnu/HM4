#ifndef PTR_WRAPPER_H
#define PTR_WRAPPER_H

template<typename T>
struct PtrWrapper{
	T *ptr;

	constexpr PtrWrapper(T *ptr) : ptr(ptr){}
	constexpr PtrWrapper(T &ptr) : ptr(&ptr){}

	constexpr
	T *get(){
		return *ptr;
	}

	constexpr
	T &operator*(){
		return *ptr;
	}

	constexpr
	T *operator->(){
		return ptr;
	}

	// ---------------------

	constexpr const T *get() const{
		return *ptr;
	}

	constexpr const T &operator*() const{
		return *ptr;
	}

	constexpr const T *operator->() const{
		return ptr;
	}
};

#endif

