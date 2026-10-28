#ifndef TIERED_MEMORY_MANAGER_H_
#define TIERED_MEMORY_MANAGER_H_

#include <stdlib.h>

namespace memory_manager{

	struct SimpleMemoryManager{

		struct Context{
			using size_type	= size_t;

			char		*data		= nullptr;
			size_type	size		= 0;
			size_type	capacity	= 0;

		public:
			constexpr void clear() noexcept{
				data		= nullptr;
				size		= 0;
				capacity	= 0;
			}

			constexpr void swap(Context &other) noexcept{
				using std::swap;

				swap(data,	other.data	);
				swap(size,	other.size	);
				swap(capacity,	other.capacity	);
			}
		};


		constexpr static std::string_view getName(){
			return "SimpleMemoryManager";
		}

		static bool allocate(Context &context, size_t needed) noexcept{
			void *p = realloc(context.data, needed);

			if (!p)
				return false;

			context.data     = static_cast<char *>(p);
			context.capacity = needed;

			return true;
		}

		static void deallocate(Context &context) noexcept{
			free(context.data);

			context.clear();
		}

	public:
		constexpr static void log() noexcept{}

		constexpr static void printShort() noexcept{}

		constexpr static void print() noexcept{}
	};



	constexpr void swap(SimpleMemoryManager::Context &a, SimpleMemoryManager::Context &b){
		a.swap(b);
	}

} // namespace memory_manager

#endif

