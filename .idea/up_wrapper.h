#ifndef MY_ALLOCATOR_UP_WRAPPER
#define MY_ALLOCATOR_UP_WRAPPER

namespace MyAllocator{

	template<class Allocator, typename T>
	static auto UPWrap(Allocator &allocator, T *p) noexcept{
		auto deleter = [&allocator](void *p){
			allocator.deallocate(p);
		};

		return std::unique_ptr<T, decltype(deleter)>{
			p,
			deleter
		};
	};

} // namespace MyAllocator

#endif

