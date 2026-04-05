#ifndef _NET_ASYNC_LOOP_SPARSE_FD_STORAGE_H_
#define _NET_ASYNC_LOOP_SPARSE_FD_STORAGE_H_

#include "sparsemap.h"

namespace net{

	namespace sparse_fd_storage_impl_{

		struct ClientFD{
			uint32_t	fd;
			Client		client;

			template<typename... Ts>
			ClientFD(uint32_t fd, Ts &&...ts) : fd(fd), client(std::forward<Ts>(ts)...){}

			constexpr auto const &operator *() const{
				return client;
			}

			constexpr auto       &operator *(){
				return client;
			}
		};

		struct SparseFDController{
			using key_type		= uint32_t;
			using mapped_type	= ClientFD;

			[[nodiscard]]
			static constexpr key_type const &getKey(mapped_type const &value){
				return value.fd;
			}
		};

	} // namespace sparse_fd_storage_impl_

	struct SparseFDStorage{
		SparseFDStorage(uint32_t conf_rlimitNoFile, uint32_t conf_maxClients) :
							clients_(conf_rlimitNoFile, conf_maxClients){}

	public:
		constexpr size_t size() const{
			return clients_.size();
		}

		auto *operator[](int fd){
			using namespace sparse_fd_storage_impl_;

			ClientFD *cfd = clients_.find(static_cast<uint32_t>(fd));

			return cfd ? & cfd->client : nullptr;
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
				if (f(static_cast<int>(x.fd), x.client))
					return;
		}

	private:
		using ClientContainer = mysparsemap::SparseMap<uint32_t, sparse_fd_storage_impl_::SparseFDController>;

		ClientContainer	clients_;
	};

} // namespace net

#endif


