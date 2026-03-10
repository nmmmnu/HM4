#ifndef _NET_ASYNC_LOOP_SLAB_FD_STORAGE_H_
#define _NET_ASYNC_LOOP_SLAB_FD_STORAGE_H_

#include "slaballocator.h"
#include "allocatedbuffer.h"

#include <vector>

namespace net{

	struct SlabFDStorage{
		SlabFDStorage(uint32_t conf_rlimitNoFile, uint32_t conf_maxClients) :
								buffer_(ClientSize * conf_maxClients){

			clients_.resize(conf_rlimitNoFile); // also set to nullptr
		}

		~SlabFDStorage(){
			for(auto *it : clients_)
				MyAllocator::destruct(allocator_, it);
		}

	public:
		constexpr size_t size() const{
			return size_;
		}

		auto *operator[](int fd){
			auto const ix = static_cast<std::size_t>(fd);

			return clients_[ix];
		}

		template<typename... Args>
		bool insert(int fd, Args&&... args){
			auto const ix = static_cast<std::size_t>(fd);

			clients_[ix] = MyAllocator::construct<Client>(allocator_, std::forward<Args>(args)...);

			++size_;

			return true;
		}

		void remove(int fd){
			auto const ix = static_cast<std::size_t>(fd);

			MyAllocator::destruct(allocator_, clients_[ix]);

			clients_[ix] = nullptr;

			--size_;
		}

		template<typename F>
		void for_each(F f){
			for(int fd = 0; fd < static_cast<int>(clients_.size()); ++fd){
				auto const ix = static_cast<std::size_t>(fd);

				if (auto *it = clients_[ix]; it)
					if (f(fd, *it))
						return;
			}
		}

	private:
		constexpr static size_t ClientSize = sizeof(Client);

		using ClientContainer	= std::vector<Client *>;
		using Buffer		= MyBuffer::AllocatedMemoryResourceOwned<>;
		using Allocator		= MyAllocator::SlabAllocator<ClientSize>;

		ClientContainer	clients_;
		size_t		size_		= 0;

		Buffer		buffer_;

		Allocator	allocator_{ buffer_ };
	};

} // namespace net

#endif


