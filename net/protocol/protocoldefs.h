#ifndef _PROTOCOL_DEFS_H
#define _PROTOCOL_DEFS_H


#include <cstdint>


namespace net{
namespace protocol{


enum class ProtocolStatus : uint8_t{
	OK,
	BUFFER_NOT_READ,
	ERROR
};


} // namespace protocol
} // namespace

#endif

