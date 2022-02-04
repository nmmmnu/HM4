#ifndef MY_ALLOCATOR_BASE
#define MY_ALLOCATOR_BASE

#include <cstddef>
#include <new>

namespace MyAllocator{

	inline void *allocate(nullptr_t, std::size_t const size){
		return ::operator new(size, std::nothrow);
	}

	inline void deallocate(nullptr_t, void *p){
		::operator delete(p);
	}



	template<class Allocator>
	void *allocate(Allocator &allocator, std::size_t const size){
		return allocator.xallocate(size);
	}

	template<class Allocator>
	void deallocate(Allocator &allocator, void *p){
		allocator.xdeallocate(p);
	}

	template<typename T, class Allocator>
	T *allocate(Allocator &allocator, std::size_t const size){
		return static_cast<T *>(
			allocate(allocator, size)
		);
	}

	template<typename T, class Allocator>
	T *allocate(Allocator &allocator){
		return allocate<T>(allocator, sizeof(T));
	}



	template<class Allocator>
	void *allocate(Allocator *allocator, std::size_t const size){
		return allocate(*allocator, size);
	}

	template<typename T, class Allocator>
	T *allocate(Allocator *allocator, std::size_t const size){
		return allocate<T>(*allocator, size);
	}

	template<typename T, class Allocator>
	T *allocate(Allocator *allocator){
		return allocate<T>(*allocator);
	}

	template<class Allocator>
	void deallocate(Allocator *allocator, void *p){
		deallocate(*allocator, p);
	}
}

#endif

