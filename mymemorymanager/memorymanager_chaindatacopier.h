#ifndef MEMORY_MANAGER_CHAIN_DATA_COPIER_H_
#define MEMORY_MANAGER_CHAIN_DATA_COPIER_H_

#include <cstring>
#include <cassert>

#include <string_view>
#include <algorithm>

namespace memory_manager{

	template<typename Node>
	struct ChainDataCopier{
		ChainDataCopier(Node *node) noexcept : node_(node){
			assert(node);
		}

		constexpr static std::string_view name(){
			return "ChainDataCopier";
		}

		void push(char c) noexcept{
			checkNext_();

			size_t const len = 1;

			memcpy(ptr_, &c, len);

			ptr_		+= len;
			node_->size	+= len;

			size_		+= len;
		}

		void push(std::string_view s) noexcept{
			const char *data	= s.data();
			auto size		= s.size();

			while (size > 0){
				checkNext_();

				size_t const len = std::min<size_t>(size, node_->capacity - node_->size);

				std::memcpy(ptr_, data, len);

				ptr_		+= len;
				node_->size	+= len;

				data		+= len;
				size		-= len;

				size_		+= len;
			}
		}

		size_t size() const noexcept{
			return size_;
		}

	private:
		void checkNext_() noexcept{
			if (node_->size < node_->capacity)
				return;

			assert(node_->next && "Chain ran out of chunks!");

			node_	= node_->next;
			ptr_	= node_->data;
		}

	private:
		Node	*node_;
		char	*ptr_	= node_->data;
		size_t	 size_	= 0;
	};

} // namespace memory_manager

#endif

