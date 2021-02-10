#ifndef _NET_SOCKETS_H
#define _NET_SOCKETS_H

#include <cstdint>

#include <unistd.h> // ssize_t

namespace net{

using options_type = uint8_t;

constexpr options_type SOCKET_NONBLOCK		= 1 << 0;
constexpr options_type SOCKET_REUSEADDR		= 1 << 1;
constexpr options_type SOCKET_TCPNODELAY	= 1 << 2;
constexpr options_type SOCKET_KEEPALIVE		= 1 << 3;
constexpr options_type SOCKET_REUSEPORT		= 1 << 4;

// ===========================

constexpr options_type SOCKET_DEFAULTOPT_TCP	= SOCKET_NONBLOCK | SOCKET_REUSEADDR | SOCKET_TCPNODELAY;
constexpr options_type SOCKET_DEFAULTOPT_UNIX	= SOCKET_NONBLOCK;

// ===========================

namespace SOCKET_ERROR{
	constexpr int CREATE	= -1;
	constexpr int OPTIONS	= -2;
	constexpr int BIND	= -3;
	constexpr int BACKLOG	= -4;
	constexpr int NAME_SIZE	= -5;
}

// ===========================

struct SOCKET_TCP{};
struct SOCKET_UNIX{};

// ===========================

int socket_create(SOCKET_TCP , const char *ip, uint16_t port, uint16_t backlog = 0, options_type options = SOCKET_DEFAULTOPT_TCP ) noexcept;
int socket_create(SOCKET_UNIX, const char *path,              uint16_t backlog = 0, options_type options = SOCKET_DEFAULTOPT_UNIX) noexcept;

bool socket_options(int fd, options_type options) noexcept;

bool socket_options_setNonBlocking	(int fd) noexcept;
bool socket_options_setReuseAddr	(int fd) noexcept;
bool socket_options_setReusePort	(int fd) noexcept;
bool socket_options_setTCPNoDelay	(int fd) noexcept;
bool socket_options_setKeepAlive	(int fd) noexcept;

int socket_accept(int fd) noexcept;

void socket_close(int fd) noexcept;

// simple wrapper functions for performance
ssize_t socket_read( int fd,       void *buf, size_t count) noexcept;
ssize_t socket_write(int fd, const void *buf, size_t count) noexcept;

ssize_t socket_read( int fd,       void *buf, size_t count, int timeout) noexcept;
ssize_t socket_write(int fd, const void *buf, size_t count, int timeout) noexcept;

bool socket_check_eagain() noexcept;

}

#endif

