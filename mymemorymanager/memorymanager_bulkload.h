#ifndef BULK_LOAD_HELPER_H_
#define BULK_LOAD_HELPER_H_

namespace memory_manager::memory_manager_impl_{

	template<typename MemoryManager, typename DataLoader>
	[[nodiscard]]
	bool bulkload(MemoryManager &mm, typename MemoryManager::NodePtr &node, DataLoader const &data) noexcept{
		mm.deallocate(node);

		auto const size = data.size();

		if (!mm.allocateBulk_(node, size))
			return false;

		using BulkLoader = typename MemoryManager::BulkLoader;

		BulkLoader bulkloader{ node };

		data.data(bulkloader);

		assert(size >= bulkloader.size() && "Buffer overflow in TieredMemoryManager::bulkLoad!");

		return true;
	}

} // namespace memory_manager

#endif

