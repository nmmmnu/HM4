#ifndef _REDIS_PROTOCOL_H
#define _REDIS_PROTOCOL_H

#include "stringref.h"

#include "protocoldefs.h"

#include <vector>
#include <utility>	// std::pair


namespace net{
namespace protocol{


class RedisProtocol{
private:
	enum class Error : uint8_t;

public:
	using Status = ProtocolStatus;

	using StringVector = std::vector<StringRef>;

public:
	RedisProtocol(){
		params_.reserve(MAX_PARAMS);
	}

public:
	Status operator()(const StringRef &src);

	const StringVector &getParams() const{
		return params_;
	}

	void print() const;

public:
	template<class CONNECTION>
	static void response_empty(CONNECTION &buffer);

	template<class CONNECTION>
	static void response_string(CONNECTION &buffer, const StringRef &msg);

	template<class CONNECTION, class CONTAINER>
	static void response_strings(CONNECTION &buffer, const CONTAINER &list);

	template<class CONNECTION>
	static void response_error(CONNECTION &buffer, const StringRef &msg);

private:
	static int readInt_(const StringRef &src, size_t &pos);
	static Status readLn_(const StringRef &src, size_t &pos);

	static std::pair<Status, StringRef> statusPair_(const Status status){
		return { status, StringRef{} };
	}

	static std::pair<Status, StringRef> errPair_(const Error ){
		return statusPair_(Status::ERROR);
	}

	static Status errStatus_(const Error ){
		return Status::ERROR;
	}

	static std::pair<Status, StringRef> readParam_(const StringRef &src, size_t &pos);

	static bool checkParamSize__(size_t const size){
		return size < 0 || size > MAX_PARAM_SIZE;
	}

private:
	constexpr static char		STAR		= '*';
	constexpr static char		DOLLAR		= '$';

	constexpr static const char	*ENDLN		= "\r\n";

private:
	constexpr static size_t	INT_BUFFER_SIZE	= 8;	// to be able to store MAX_PARAM_SIZE as string.
	constexpr static size_t	MAX_PARAMS	= 4;	// setex name 100 hello
	constexpr static size_t	MAX_PARAM_SIZE	= 16 * 1024;

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
void RedisProtocol::response_string(CONNECTION &buffer, const StringRef &msg){
	buffer.push(DOLLAR);
	buffer.push(std::to_string(msg.size()));
	buffer.push(ENDLN);

	buffer.push(msg);
	buffer.push(ENDLN);
}

template<class CONNECTION, class CONTAINER>
void RedisProtocol::response_strings(CONNECTION &buffer, const CONTAINER &list){
	buffer.push(STAR);
	buffer.push(std::to_string(list.size()));
	buffer.push(ENDLN);

	for(const auto &msg : list)
		response_string(buffer, msg);
}

template<class CONNECTION>
void RedisProtocol::response_error(CONNECTION &buffer, const StringRef &msg){
	buffer.push("-ERR ");
	buffer.push(msg);
	buffer.push(ENDLN);
}


} // namespace protocol
} // namespace

#endif

