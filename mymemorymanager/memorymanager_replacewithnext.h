#ifndef MEMORY_MANAGER_GET_NEXT_
#define MEMORY_MANAGER_GET_NEXT_

namespace memory_manager{

	template<typename MemoryManager>
	bool replaceWithNext(MemoryManager &mm, typename MemoryManager::NodePtr &node){
		if (!node->next){
			mm.deallocate(node);

			return false;
		}else{
			auto *pnode = node;

			node = pnode->next;

			pnode->next = nullptr;

			mm.deallocate(pnode);

			return true;
		}
	}

}

#endif

