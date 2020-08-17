#ifndef _REDIS_PROTOCOL_H
#define _REDIS_PROTOCOL_H

#include "protocoldefs.h"

#include <string>
#include <string_view>
#include <utility>	// std::pair

#include "mystring.h"

#include "fixedvector.h"

namespace net{
namespace protocol{


class RedisProtocol{
public:
	constexpr static char		STAR		= '*';
	constexpr static char		DOLLAR		= '$';

	constexpr static const char	*ENDLN		= "\r\n";

public:
	constexpr static size_t	MAX_PARAMS	= 4;	// setex name 100 hello
							// getx  name 100 prefix

public:
	using Status = ProtocolStatus;

	#if 0
	using StringVector = std::vector<std::string_view>;
	#else
	using StringVector = FixedVector<std::string_view, MAX_PARAMS>;
	#endif

public:
	RedisProtocol(){
		params_.reserve(MAX_PARAMS);
	}

public:
	Status operator()(std::string_view const src);

	const StringVector &getParams() const{
		return params_;
	}

	void print() const;

public:
	template<class CONNECTION>
	static void response_empty(CONNECTION &buffer);

	template<class CONNECTION>
	static void response_ok(CONNECTION &buffer);

	template<class CONNECTION>
	static void response_error(CONNECTION &buffer, std::string_view msg);

	template<class CONNECTION>
	static void response_bool(CONNECTION &buffer, bool b);

	template<class CONNECTION>
	static void response_string(CONNECTION &buffer, std::string_view msg);

	template<class CONNECTION, class CONTAINER>
	static void response_strings(CONNECTION &buffer, const CONTAINER &list);

	template<class CONNECTION>
	static void response_strings(CONNECTION &buffer, std::string_view msg1, std::string_view msg2);

public:
	template<class CONNECTION>
	static void response_string(CONNECTION &buffer, uint64_t const msg){
		// if we change uint64_t to template, std::string will come here.
		return response_string(buffer, std::to_string(msg));
	}

private:
	StringVector	params_;
};


// ==================================


template<class CONNECTION>
void RedisProtocol::response_empty(CONNECTION &buffer){
	buffer.push("$-1");
	buffer.push(ENDLN);
}

template<class CONNECTION>
void RedisProtocol::response_ok(CONNECTION &buffer){
	buffer.push("+OK");
	buffer.push(ENDLN);
}

template<class CONNECTION>
void RedisProtocol::response_error(CONNECTION &buffer, std::string_view const msg){
	buffer.push("-ERR ");
	buffer.push(msg);
	buffer.push(ENDLN);
}

template<class CONNECTION>
void RedisProtocol::response_bool(CONNECTION &buffer, bool const b){
	buffer.push(b ? ":1" : ":0");
	buffer.push(ENDLN);
}

template<class CONNECTION>
void RedisProtocol::response_string(CONNECTION &buffer, std::string_view const msg){
	to_string_buffer_t mybuffer;

	buffer.push(DOLLAR);
	buffer.push(to_string(msg.size(), mybuffer));
	buffer.push(ENDLN);

	buffer.push(msg);
	buffer.push(ENDLN);
}

template<class CONNECTION, class CONTAINER>
void RedisProtocol::response_strings(CONNECTION &buffer, const CONTAINER &list){
	to_string_buffer_t mybuffer;

	buffer.push(STAR);
	buffer.push(to_string(list.size(), mybuffer));
	buffer.push(ENDLN);

	for(const auto &msg : list)
		response_string(buffer, msg);
}

template<class CONNECTION>
void RedisProtocol::response_strings(CONNECTION &buffer, std::string_view const msg1, std::string_view const msg2){
	constexpr std::string_view count = "2";

	buffer.push(STAR);
	buffer.push(count);
	buffer.push(ENDLN);

	response_string(buffer, msg1);
	response_string(buffer, msg2);
}


} // namespace protocol
} // namespace

#endif

