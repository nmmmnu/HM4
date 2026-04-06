#ifndef _NET_ASYNC_LOOP_SPARSE_FD_STORAGE_H_
#define _NET_ASYNC_LOOP_SPARSE_FD_STORAGE_H_

#include "sparsearray/easymap.h"

namespace net{

	struct SparseFDStorage{
		SparseFDStorage(uint32_t conf_rlimitNoFile, uint32_t conf_maxClients) :
							clients_(conf_rlimitNoFile, conf_maxClients){}

	public:
		constexpr size_t size() const{
			return clients_.size();
		}

		auto *operator[](int fd){
			return clients_.find(static_cast<uint32_t>(fd));
		}

		template<typename... Args>
		bool insert(int fd, Args&&... args){
			return clients_.insert(static_cast<uint32_t>(fd), std::forward<Args>(args)...);
		}

		void remove(int fd){
			clients_.remove(static_cast<uint32_t>(fd));
		}

		template<typename F>
		void for_each(F f){
			for(auto &x : clients_)
				if (f(static_cast<int>(x.first), x.second))
					return;
		}

	private:
		using ClientContainer = mysparsearray::EasyMap<uint32_t, Client>;

		ClientContainer	clients_;
	};

} // namespace net

#endif


