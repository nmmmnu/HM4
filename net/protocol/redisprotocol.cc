#include "redisprotocol.h"

#include <iostream>

namespace net{
namespace protocol{


class RedisProtocolParser{
public:
	using Status		= RedisProtocol::Status;
	using StringVector	= RedisProtocol::StringVector;

private:
	enum class Error : uint8_t{
		NO_STAR,
		NO_DOLLAR,
		PARAM_COUNT,
		PARAM_SIZE,
		NO_CRLF
	};

private:
	constexpr static auto	STAR		= RedisProtocol::STAR;
	constexpr static auto	DOLLAR		= RedisProtocol::DOLLAR;
	constexpr static auto	ENDLN		= RedisProtocol::ENDLN;

	constexpr static auto	MAX_PARAMS	= RedisProtocol::MAX_PARAMS;

private:
	constexpr static size_t PAIR_MAX_VAL_SIZE	= 0b0'0000'1111'1111'1111'1111'1111'1111'1111;
	constexpr static size_t	MAX_PARAM_SIZE		= PAIR_MAX_VAL_SIZE;	// same or greater than max value
	constexpr static size_t	INT_BUFFER_SIZE		= 16;			// to be able to store MAX_PARAM_SIZE as string.

public:
	RedisProtocolParser(std::string_view const src, StringVector &params) : src(src), params(params){}

	Status operator()(){
		if (pos + 2 > src.size()){
			// buffer is definitely not read,
			// should be at least *1
			return Status::BUFFER_NOT_READ;
		}

		// pos = 0;

		if (src[pos] != STAR)
			return errStatus_(Error::NO_STAR);

		++pos;

		size_t const paramsCount = static_cast<size_t>(readInt_());

		if (paramsCount == 0 || paramsCount > MAX_PARAMS)
			return errStatus_(Error::PARAM_COUNT);

		params.clear();

		{
			const Status stat = readLn_();
			if ( stat != Status::OK )
				return stat;
		}

		for(size_t i = 0; i < paramsCount; ++i){
			const auto & [status, data] = readParam_();

			if ( status == Status::OK )
				params.push_back(data);
			else
				return status;
		}

		return Status::OK;
	}

private:
	int readInt_(){
		char buff[INT_BUFFER_SIZE + 1];

		size_t bpos = 0;

		while(pos < src.size() && bpos < INT_BUFFER_SIZE){
			char const c = src[pos];
			if (c != '-' && c != '+' && (c < '0' || c > '9') )
				break;

			buff[bpos] = c;

			++pos;
			++bpos;
		}

		buff[bpos] = '\0';

		return atoi(buff);
	}

	Status readLn_(){
		if (pos + 2 > src.size())
			return Status::BUFFER_NOT_READ;

		if (src[pos] == '\r' && src[pos + 1] == '\n'){
			pos += 2;
			return Status::OK;
		}

		return errStatus_(Error::NO_CRLF);
	}

	std::pair<Status, std::string_view> readParam_(){
		if (pos + 2 > src.size()){
			// buffer is definitely not read,
			// should be at least $0
			return statusPair_(Status::BUFFER_NOT_READ);
		}

		if (src[pos] != DOLLAR)
			return errPair_(Error::NO_DOLLAR);

		++pos;

		const size_t size = static_cast<size_t>(readInt_());

		if (/* size < 0 || */ size > MAX_PARAM_SIZE)
			return errPair_(Error::PARAM_SIZE);



		if (auto const stat = readLn_(); stat != Status::OK )
			return statusPair_(stat);



		const auto pos_save = pos;

		// store param must be here, but is optimized at the end.

		pos += size;

		// check if all data is there.
		if (pos > src.size())
			return statusPair_(Status::BUFFER_NOT_READ);



		if (auto const stat = readLn_(); stat != Status::OK )
			return statusPair_(stat);



		return { Status::OK, std::string_view( & src[pos_save], size) };
	}

private:
	constexpr static Status errStatus_(Error ){
		return Status::ERROR;
	}

	constexpr static std::pair<Status, std::string_view> errPair_(Error ){
		return statusPair_(Status::ERROR);
	}

	constexpr static std::pair<Status, std::string_view> statusPair_(const Status status){
		return { status, std::string_view{} };
	}

private:
	std::string_view	src;
	StringVector		&params;
	size_t			pos = 0;
};


// ==================================


auto RedisProtocol::operator()(std::string_view const src) -> Status{
	// this doing more harm than relps.
//	if (src.size() < 8)	// 4 bytes - "*1\r\n$1\r\n"
//		return Status::BUFFER_NOT_READ;

	RedisProtocolParser parser(src, params_);

	return parser();
}


void RedisProtocol::print() const{
	for(const auto &item : params_ )
		std::cout << item << '\n';
}


} // namespace protocol
} // namespace

