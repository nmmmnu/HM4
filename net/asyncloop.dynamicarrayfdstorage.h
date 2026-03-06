#ifndef _NET_ASYNC_LOOP_DYNAMIC_ARRAY_STORAGE_H_
#define _NET_ASYNC_LOOP_DYNAMIC_ARRAY_STORAGE_H_

#include "stdallocator.h"

#include <vector>

namespace net{

	struct DynamicArrayFDStorage{
		DynamicArrayFDStorage(uint32_t conf_rlimitNoFile, uint32_t /* conf_maxClients */){
			clients_.resize(conf_rlimitNoFile); // also set to nullptr
		}

		~DynamicArrayFDStorage(){
			for(auto *it : clients_)
				MyAllocator::destruct(allocator__, it);
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
			clients_[fd] = MyAllocator::construct<Client>(allocator__, std::forward<Args>(args)...);

			++size_;

			return true;
		}

		void remove(int fd){
			MyAllocator::destruct(allocator__, clients_[fd]);

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
		using ClientContainer	= std::vector<Client *>;

		ClientContainer	clients_;
		size_t		size_		= 0;

		constexpr inline static MyAllocator::STDAllocator
				allocator__{};
	};

} // namespace net

#endif


