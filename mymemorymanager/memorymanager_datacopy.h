#ifndef MEMORY_MANAGER_DATA_COPY_H_
#define MEMORY_MANAGER_DATA_COPY_H_

#include "memorymanager_simpledatacopier.h"
#include "memorymanager_chaindatacopier.h"

#include "logger.h"

namespace memory_manager::memory_manager_impl_{

	template<typename DataCopier, typename Node, typename DataSource>
	[[nodiscard]]
	bool dataCopy_(Node *node, DataSource const &dataSource, size_t allocatedSize){
		logger<Logger::Level::DEBUG>() << "data loading"<< allocatedSize << "bytes into IOBuffer with" << DataCopier::name();

		DataCopier dataCopier{ node };

		dataSource.data(dataCopier);

		if (dataCopier.size() > allocatedSize){
			logger<Logger::Level::FATAL>() << "Buffer overflow in TieredMemoryManager::dataCopy() !";
			assert(false && "Buffer overflow in TieredMemoryManager::bulkLoad!");
		}

		return true;
	}

	template<typename MemoryManager, typename DataSource>
	[[nodiscard]]
	bool dataCopy(MemoryManager &mm, typename MemoryManager::NodePtr &node, DataSource const &dataSource) noexcept{
		mm.deallocate(node);

		auto const size = dataSource.size();

		if (!mm.allocateBulk(node, size))
			return false;

		using Node = typename MemoryManager::Node;

		if constexpr(!Node::SUPPORT_CHAIN){
				return dataCopy_<SimpleDataCopier<Node>	>(node, dataSource, size);
		}else{
			if (!node->next)
				return dataCopy_<SimpleDataCopier<Node>	>(node, dataSource, size);
			else
				return dataCopy_<ChainDataCopier<Node>	>(node, dataSource, size);
		}

	}

} // namespace memory_manager

#endif

