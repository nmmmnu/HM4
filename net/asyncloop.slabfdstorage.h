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
			return clients_[fd];
		}

	//	const auto *operator[](int fd) const{
	//		return clients_[fd];
	//	}

		template<typename... Args>
		bool insert(int fd, Args&&... args){
			clients_[fd] = MyAllocator::construct<Client>(allocator_, std::forward<Args>(args)...);

			++size_;

			return true;
		}

		void remove(int fd){
			MyAllocator::destruct(allocator_, clients_[fd]);

			clients_[fd] = nullptr;

			--size_;
		}

		template<typename F>
		void for_each(F f){
			for(int fd = 0; fd < static_cast<int>(clients_.size()); ++fd)
				if (auto *it = clients_[fd]; it)
					if (f(fd, *it))
						return;
		}

	private:
		constexpr static size_t ClientSize = sizeof(Client);

		using ClientContainer	= std::vector<Client *>;
		using Allocator		= MyAllocator::SlabAllocator<ClientSize>;

		ClientContainer	clients_;
		size_t		size_		= 0;

		MyBuffer::AllocatedMemoryResourceOwned<>
				buffer_;

		Allocator	allocator_{ buffer_ };
	};

} // namespace net

#endif


