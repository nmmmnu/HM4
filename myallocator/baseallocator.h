#ifndef MY_BASE_ALLOCATOR_H_
#define MY_BASE_ALLOCATOR_H_

#include <cstddef>
//#include <new>
#include <memory>
#include <limits>

namespace MyAllocator{

	inline void *allocate(std::nullptr_t, std::size_t const size){
		return ::operator new(size, std::nothrow);
	}

	inline void deallocate(std::nullptr_t, void *p){
		::operator delete(p);
	}

	template<typename T>
	auto wrapInSmartPtr(std::nullptr_t &, T *p) noexcept{
		return std::unique_ptr<T>{ p };
	}



	template<class Allocator>
	void *allocate(Allocator &allocator, std::size_t const size){
		return allocator.xallocate(size);
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
	void deallocate(Allocator &allocator, void *p){
		allocator.xdeallocate(p);
	}



	template<typename T, class Allocator>
	auto wrapInSmartPtr(Allocator &allocator, T *p) noexcept{
		auto deleter = [&](void *p){
			allocator.xdeallocate(p);
		};

		return std::unique_ptr<T, decltype(deleter)>{
			p,
			deleter
		};
	}

	template<typename T, class Allocator>
	using SmartPtrType = decltype(
		// because of polymorphic classes,
		// this needs to be done in this way.
		// then in each class that require different deleter,
		// you need to add wrapInSmartPtr() function

		wrapInSmartPtr(
			std::declval<Allocator &>(),
			std::declval<T *>()
		)
	);



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

