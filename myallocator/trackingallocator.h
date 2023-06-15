#ifndef MY_TRACKING_ALLOCATOR
#define MY_TRACKING_ALLOCATOR

#include "baseallocator.h"
#include "malloc_usable_size.h"
#include "mallocallocator.h"
#include "stdallocator.h"

#define FMT_HEADER_ONLY
#include "fmt/core.h"

namespace MyAllocator{

	template<class Allocator>
	struct TrackingAllocator{
		const char *getName() const{
			return allocator.getName();
		}

		constexpr static const char *TAG = "[TrackingAllocator]";

		template<class ...Args>
		TrackingAllocator(Args &&...args) : allocator( std::forward<Args>(args)... ){}

		TrackingAllocator(TrackingAllocator &&other) :
					allocator	(std::move(other.allocator	)),
					allocated	(std::move(other.allocated	)),
					deallocated	(std::move(other.deallocated	)),
					allocations	(std::move(other.allocations	)),
					deallocations	(std::move(other.deallocations	)),
					printSummary	(std::move(other.printSummary	)){

			other.printSummary = false;
		}

		~TrackingAllocator(){
			if (!printSummary)
				return;

			const char *mask = "{} Total {:16} {:8}\n";

			fmt::print(mask, TAG, "Allocated Size",		allocated			);
			fmt::print(mask, TAG, "Deallocated Size",	deallocated			);
			fmt::print(mask, TAG, "Allocations",		allocations			);
			fmt::print(mask, TAG, "Dellocations",		deallocations			);
			fmt::print(mask, TAG, "Lost bytes",		deallocated - deallocated	);
			fmt::print(mask, TAG, "Lost count",		allocations - deallocations	);
		}

		void *xallocate(std::size_t const size) noexcept{
			++allocations;
			void *p = allocator.xallocate(size);
			allocated += malloc_usable_size__(p);

			fmt::print("{} Allocate {:8} -> {}\n", TAG, size, p);
			return p;
		}

		void xdeallocate(void *p) noexcept{
			++deallocations;
			deallocated += malloc_usable_size__(p);
			fmt::print("{} Deallocate {}\n", TAG, p);
			return allocator.xdeallocate(p);
		}

		bool owns(const void *p) const noexcept{
			return allocator.owns(p);
		}

		bool need_deallocate() const noexcept{
			return allocator.need_deallocate();
		}

		bool knownMemoryUsage() const noexcept{
			return allocator.knownMemoryUsage();
		}

		bool reset() noexcept{
			return allocator.reset();
		}

		size_t getFreeMemory() const noexcept{
			return allocator.getFreeMemory();
		}

		size_t getUsedMemory() const noexcept{
			return allocated;
		}

	private:
		static size_t malloc_usable_size__(void *) noexcept{
			return 0;
		}

	private:
		Allocator	allocator;
		size_t		allocated	= 0;
		size_t		deallocated	= 0;
		size_t		allocations	= 0;
		size_t		deallocations	= 0;
		bool		printSummary	= true;
	};

	template<>
	size_t TrackingAllocator<MallocAllocator>::malloc_usable_size__(void *p) noexcept{
		return malloc_usable_size(p);
	}

	template<>
	size_t TrackingAllocator<STDAllocator>::malloc_usable_size__(void *p) noexcept{
		return malloc_usable_size(p);
	}

} // namespace MyAllocator

#endif

