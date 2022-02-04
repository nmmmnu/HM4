#ifndef MY_ALLOCATOR_BASE
#define MY_ALLOCATOR_BASE

#include <cstddef>
#include <new>

namespace MyAllocator{

	inline void *allocate(std::nullptr_t, std::size_t const size){
		return ::operator new(size, std::nothrow);
	}

	inline void deallocate(std::nullptr_t, void *p){
		::operator delete(p);
	}

	template<typename T>
	static auto wrapInSmartPtr(std::nullptr_t &, T *p) noexcept{
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



	namespace SmartPtrWrapper_{
		template<class Allocator>
		struct Wrapper{
			template<typename T>
			static auto make(Allocator &allocator, T *p) noexcept{
				auto deleter = [&](void *p){
					allocator.xdeallocate(p);
				};

				return std::unique_ptr<T, decltype(deleter)>{
					p,
					deleter
				};
			}
		};
	}



	// Because of polymorphic types,
	// this function must be partially specialized.
	// This is why a helper class is used.
	template<typename T, class Allocator>
	static auto wrapInSmartPtr(Allocator &allocator, T *p) noexcept{
		return SmartPtrWrapper_::Wrapper<Allocator>::make(allocator, p);
	}

	template<typename T, class Allocator>
	using SmartPtrType = decltype(
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

