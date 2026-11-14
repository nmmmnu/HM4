#ifndef MEMORY_MANAGER_SIMPLE_DATA_COPIER_H_
#define MEMORY_MANAGER_SIMPLE_DATA_COPIER_H_

#include <cstring>
#include <cassert>

#include <string_view>

namespace memory_manager{

	template<typename Node>
	struct SimpleDataCopier{
		SimpleDataCopier(Node *node) noexcept : node(node){
			assert(node);
			assert(node->next == nullptr && "SimpleDataCopier can no work with chains!");
		}

		constexpr static std::string_view name(){
			return "SimpleDataCopier";
		}

		void push(char c) noexcept{
			return push(std::string_view{ &c, 1 });
		}

		void push(std::string_view s) noexcept{
			assert(node->capacity - node->size >= s.size() && "Node ran out of space!");

			std::memcpy(node->data + node->size, s.data(), s.size());

			node->size += s.size();
		}

		size_t size() const noexcept{
			return node->size;
		}

	private:
		Node *node;
	};

} // namespace memory_manager

#endif

