#ifndef SIMPLE_BULK_LOADER_H_
#define SIMPLE_BULK_LOADER_H_

#include <cstring>
#include <cassert>

#include <string_view>

namespace memory_manager{

	template<typename NodePtr>
	struct SimpleBulkLoader{
		SimpleBulkLoader(NodePtr &node) noexcept : node(node){
			assert(node);
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
		NodePtr &node;
	};

} // namespace memory_manager

#endif

