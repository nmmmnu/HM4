#ifndef MEMORY_MANAGER_GET_NEXT_
#define MEMORY_MANAGER_GET_NEXT_

namespace memory_manager{

	template<typename MemoryManager>
	bool replaceWithNext(MemoryManager &mm, typename MemoryManager::NodePtr &node){
		static_assert(MemoryManager::Node::SUPPORT_CHAIN);

		if (!node->next || node->next->size == 0){
			mm.deallocate(node);
			return false;
		}

		auto *pnode = node;
		node = pnode->next;
		pnode->next = nullptr;

		mm.deallocate(pnode);
		return true;
	}

}// namespace memory_manager

#endif

