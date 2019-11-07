#ifndef MY_TRACKING_ALLOCATOR
#define MY_TRACKING_ALLOCATOR

#include  <cstddef>
#include  <cstdio>

namespace MyAllocator{

	template<class Allocator>
	struct TrackingAllocator{
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

			const char *mask = "%s Total %-16s %8zu\n";
			printf(mask, TAG, "Allocated Size",	allocated			);
			printf(mask, TAG, "Allocations",	allocations			);
			printf(mask, TAG, "Dellocations",	deallocations			);
			printf(mask, TAG, "Lost",		allocations - deallocations	);
		}

		void *allocate(std::size_t const size) noexcept{
			allocated += size;
			++allocations;
			void *p = a.allocate(size);
			printf("%s Allocate %8zu -> %p\n", TAG, size, p);
			return p;
		}

		void deallocate(void *p) noexcept{
			++deallocations;
			printf("%s Deallocate %p\n", TAG, p);
			return a.deallocate(p);
		}

		bool need_deallocate() const noexcept{
			return a.need_deallocate();
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

