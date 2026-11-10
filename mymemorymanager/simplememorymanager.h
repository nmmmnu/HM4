#ifndef SIMPLE_MEMORY_MANAGER_H_
#define SIMPLE_MEMORY_MANAGER_H_

#include <cstdlib>
#include <cassert>
#include <cstddef>	// offsetof
#include <cstdint>

namespace memory_manager{

	struct SimpleMemoryManager{
		using size_type = uint32_t;

		struct Node{
			size_type	size		; // 4
			size_type	capacity	; // 4
			size_type	head		; // 4
			char		padding[4]	; // 4

			char		data[4]		; // 1 + 3 pad
		};

		using NodePtr = Node *;

		constexpr static size_t NODE_SIZE = offsetof(Node, data);

		constexpr static std::string_view getName() noexcept{
			return "SimpleMemoryManager";
		}
		static bool reallocate(NodePtr &node, size_t needed) noexcept{
			if (!node){
				// case 1 - new allocation

				size_t const size = needed + NODE_SIZE;

				void *p = malloc(size);

				if (!p)
					return false;

				node		= static_cast<Node *>(p);

				node->size	= 0;
				node->capacity	= static_cast<uint32_t>(needed);
				node->head	= 0;

				return true;
			}if (needed <= node->capacity){
				// case 2 - no work need.

				return true;
			}else{
				// case 3 - realloc

				size_t const size = needed + NODE_SIZE;

				void *p = realloc(node, size);

				if (!p)
					return false;

				node		= static_cast<Node *>(p);
				node->capacity	= static_cast<uint32_t>(needed);

				return true;
			}
		}

		static void deallocate(NodePtr &node) noexcept{
			free(node);

			node = nullptr;
		}

	public:
		template<Logger::Level Level>
		constexpr static void log()		noexcept{}
		constexpr static void printShort()	noexcept{}
		constexpr static void print()		noexcept{}
	};

} // namespace memory_manager

#endif

