#ifndef _NET_ASYNC_LOOP_MAP_FD_STORAGE_H_
#define _NET_ASYNC_LOOP_MAP_FD_STORAGE_H_

#include "stdallocator.h"

#include <unordered_map>

namespace net{

	// Historical Storage using unordered_map, made for completness.

	template<typename Client>
	struct FDMapStorage{
		FDMapStorage(uint32_t /* conf_rlimitNoFile */, uint32_t /* conf_maxClients */){}

	public:
		size_t size() const{
			return clients_.size();
		}

	//	bool operator()(int fd) const{
	//		return clients_[fd];
	//	}

		auto *operator[](int fd){
			auto it = clients_.find(fd);

			return it == clients_.end() ? nullptr : & it->second;
		}

		template<typename... Args>
		bool insert(int fd, Args&&... args){
			clients_.emplace(fd, std::forward<Args>(args)...);

			return true;
		}

		void remove(int fd){
			clients_.erase(fd);
		}

		template<typename F>
		void for_each(F f){
			for(auto &[fd, client] : clients_)
				if (f(fd, client))
					return;
		}

	private:
		using ClientContainer	= std::unordered_map<int, Client>;

		ClientContainer	clients_;
	};

} // namespace net

#endif


