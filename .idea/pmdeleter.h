#ifndef MY_PM_DELETER_H
#define MY_PM_DELETER_H

#include <cstdint>

namespace MyAllocator{

	struct PMDeleter{
		virtual void operator()(void *) = 0;
		virtual ~PMDeleter() = default;
	};

	template<class Allocator>
	struct PMSDeleter : PMDeleter {
		void operator()(void *p) override final{
			return Allocator::deallocate(p);
		}
	};



	template<class Allocator>
	struct AllocatorPMSDeleter : Allocator{
		PMDeleter *getDeleter(){
			return &deleter;
		}

	private:
		PMSDeleter<Allocator>	deleter;
	};


} // namespace MyAllocator

#endif

