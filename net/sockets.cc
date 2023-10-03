#include "sockets.h"

#include <sys/socket.h>		// EAGAIN
#include <arpa/inet.h>		// AF_INET
#include <sys/un.h>		// AF_UNIX
#include <netinet/tcp.h>	// TCP_NODELAY
#include <netinet/in.h>		// IPPROTO_TCP for FreeBSD
#include <fcntl.h>		// fcntl
#include <string.h>		// strlen
#include <unistd.h>		// close, read, write

#include <errno.h>		// errno

#include <poll.h>

namespace net{

using pollfd_event_type = decltype(pollfd::events);

// ===========================

inline bool socket_poll_(int const fd, pollfd_event_type event, int const timeout);

inline int socket_error_(int const fd, int const error);

template <class SOCKADDR>
int socket_server_(int const fd, SOCKADDR &address, uint16_t const backlog);

// ===========================

bool socket_check_eagain() noexcept{
	return errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK;
}

bool socket_options_setNonBlocking(int const fd) noexcept{
	return fcntl(fd, F_SETFL, O_NONBLOCK) >= 0;
}

template<int Proto, int Opt>
bool socket_setOption_(int const fd, int const val = 1){
	return setsockopt(fd, Proto, Opt, & val, sizeof val) >= 0;
}


bool socket_options_setReuseAddr(int const fd) noexcept{
	return socket_setOption_<SOL_SOCKET, SO_REUSEADDR>(fd);
}

bool socket_options_setReusePort(int const fd) noexcept{
	#ifdef SO_REUSEPORT
	return socket_setOption_<SOL_SOCKET, SO_REUSEPORT>(fd);
	#else
	return false;
	#endif
}

bool socket_options_setTCPNoDelay(int const fd) noexcept{
	return socket_setOption_<IPPROTO_TCP, TCP_NODELAY>(fd);
}

bool socket_options_setKeepAlive(int const fd) noexcept{
	return socket_setOption_<SOL_SOCKET, SO_KEEPALIVE>(fd);
}

void socket_close(int const fd) noexcept{
	::close(fd);
}

ssize_t socket_read(int const fd, void *buf, size_t const count) noexcept{
	return ::read(fd, buf, count);
}

ssize_t socket_write(int const fd, const void *buf, size_t const count) noexcept{
	return ::write(fd, buf, count);
}

int socket_accept(int const fd) noexcept{
	return ::accept(fd, nullptr, nullptr);
}

// ===========================

ssize_t socket_read(int const fd, void *buf, size_t const count, int const timeout) noexcept{
	bool const ready = socket_poll_(fd, POLLIN, timeout);

	if (!ready){
		// simulate EAGAIN
		errno = EAGAIN;
		return -1;
	}

	return socket_read(fd, buf, count);
}

ssize_t socket_write(int const fd, const void *buf, size_t const count, int const timeout) noexcept{
	bool const ready = socket_poll_(fd, POLLOUT, timeout);

	if (!ready){
		// simulate EAGAIN
		errno = EAGAIN;
		return -1;
	}

	return socket_write(fd, buf, count);
}

// ===========================

bool socket_options(int fd, options_type const options) noexcept{
	if (options & SOCKET_REUSEADDR	&& ! socket_options_setReuseAddr(fd) )
		return false;

	if (options & SOCKET_REUSEPORT	&& ! socket_options_setReusePort(fd) )
		return false;

	if (options & SOCKET_NONBLOCK	&& ! socket_options_setNonBlocking(fd) )
		return false;

	if (options & SOCKET_TCPNODELAY	&& ! socket_options_setTCPNoDelay(fd) )
		return false;

	if (options & SOCKET_KEEPALIVE	&& ! socket_options_setKeepAlive(fd) )
		return false;

	return true;
}

int socket_create(SOCKET_TCP, const char *ip, uint16_t const port, uint16_t const backlog, options_type const options) noexcept{
	(void) ip;

	int fd = socket(AF_INET , SOCK_STREAM , 0);

	if(fd < 0)
		return SOCKET_ERROR::CREATE;

	if (!socket_options(fd, options))
		return socket_error_(fd, SOCKET_ERROR::OPTIONS);

	struct sockaddr_in address;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	return socket_server_(fd, address, backlog);
}

int socket_create(SOCKET_UNIX, const char *path, uint16_t const backlog, options_type const options) noexcept{
	if (sizeof sockaddr_un::sun_path < strlen(path))
		return SOCKET_ERROR::NAME_SIZE;


	int fd = socket(AF_UNIX , SOCK_STREAM , 0);

	if(fd < 0)
		return SOCKET_ERROR::CREATE;

	if (!socket_options(fd, options))
		return socket_error_(fd, SOCKET_ERROR::OPTIONS);

	struct sockaddr_un address;
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, path);

	unlink(address.sun_path);

	return socket_server_(fd, address, backlog);
}

// ===========================

inline bool socket_poll_(int const fd, pollfd_event_type event, int const timeout){
	pollfd p;
	p.fd		= fd;
	p.events	= event;

	// size cast is for FreeBSD and OSX warning
	int const result = poll(&p, (nfds_t) 1, timeout);

	// we are still checking the result, because it could be -1
	return result == 1 && p.revents & event;
}

// ===========================

template <class SOCKADDR>
int socket_server_(int const fd, SOCKADDR &address, uint16_t const backlog){
	if (bind(fd, (struct sockaddr *) & address, sizeof address) < 0){
		::close(fd);
		return SOCKET_ERROR::BIND;
	}

	if (listen(fd, backlog ? backlog : SOMAXCONN) < 0){
		::close(fd);
		return SOCKET_ERROR::BACKLOG;
	}

	return fd;
}

inline int socket_error_(int const fd, int const error){
	::close(fd);
	return error;
}

} // namespace

