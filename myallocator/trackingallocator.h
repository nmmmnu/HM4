#ifndef MY_TRACKING_ALLOCATOR
#define MY_TRACKING_ALLOCATOR

#include "allocator_base.h"
#define FMT_HEADER_ONLY
#include "fmt/printf.h"

namespace MyAllocator{

	template<class Allocator>
	struct TrackingAllocator{
		const char *getName() const{
			return a.getName();
		}

		constexpr static const char *TAG = "[TrackingAllocator]";

		TrackingAllocator() = default;

		TrackingAllocator(TrackingAllocator &&other) :
					a		(std::move(other.a		)),
					allocated	(std::move(other.allocated	)),
					allocations	(std::move(other.allocations	)),
					deallocations	(std::move(other.deallocations	)),
					printSummary	(std::move(other.printSummary	)){

			other.printSummary = false;
		}

		~TrackingAllocator(){
			if (!printSummary)
				return;

			const char *mask = "{} Total {:16} {:8}\n";

			fmt::print(mask, TAG, "Allocated Size",	allocated			);
			fmt::print(mask, TAG, "Allocations",	allocations			);
			fmt::print(mask, TAG, "Dellocations",	deallocations			);
			fmt::print(mask, TAG, "Lost",		allocations - deallocations	);
		}

		void *xallocate(std::size_t const size) noexcept{
			allocated += size;
			++allocations;
			void *p = a.xallocate(size);
			fmt::print("{} Allocate {:8} -> {}\n", TAG, size, p);
			return p;
		}

		void xdeallocate(void *p) noexcept{
			++deallocations;
			fmt::print("{} Deallocate {}\n", TAG, p);
			return a.xdeallocate(p);
		}

		bool need_deallocate() const noexcept{
			return a.need_deallocate();
		}

		bool knownMemoryUsage() const noexcept{
			return a.knownMemoryUsage();
		}

		bool reset() noexcept{
			return a.reset();
		}

		std::size_t getFreeMemory() const noexcept{
			return a.getFreeMemory();
		}

		std::size_t getUsedMemory() const noexcept{
			return allocated;
		}

	private:
		Allocator a;
		std::size_t allocated		= 0;
		std::size_t allocations		= 0;
		std::size_t deallocations	= 0;
		bool printSummary		= true;
	};

} // namespace MyAllocator

#endif

