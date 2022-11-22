#ifndef _PROTOCOL_DEFS_H
#define _PROTOCOL_DEFS_H



#include <cstdint>
#include <string_view>



namespace net{
namespace protocol{



enum class ProtocolStatus : uint8_t{
	OK,
	BUFFER_NOT_READ,
	ERROR
};



constexpr std::string_view toString(ProtocolStatus status){
	switch(status){
	case ProtocolStatus::OK			: return "OK"			;
	case ProtocolStatus::BUFFER_NOT_READ	: return "BUFFER_NOT_READ"	;
	case ProtocolStatus::ERROR		: return "ERROR"		;
	default					: return "[something_else]"	;
	}
}



} // namespace protocol
} // namespace

#endif

