#ifndef WRAPPER_ALLOCATOR
#define WRAPPER_ALLOCATOR

namespace MyAllocator{

	template<class Allocator, typename T>
	static auto wrapInUniquePtr(Allocator &allocator, T *p) noexcept{
		auto deleter = [&allocator](void *p){
			allocator.deallocate(p);
		};

		return std::unique_ptr<T, decltype(deleter)>{
			p,
			deleter
		};
	}

	template<typename T>
	static auto wrapInUniquePtr(unique_ptr_allocator &, T *p) noexcept{
		return std::unique_ptr<T>{ p };
	}

} // namespace MyAllocator

#endif

