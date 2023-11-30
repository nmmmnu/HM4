#ifndef _REDIS_PROTOCOL_H
#define _REDIS_PROTOCOL_H

#include "protocoldefs.h"

#include <string>
#include <numeric>	// std::accumulate
#include <utility>	// std::pair
#include <string_view>

#include "mystring.h"

#include "staticvector.h"
#include "myspan.h"

#include "logger.h"



namespace net{
namespace protocol{



namespace redis_protocol_impl_{
	constexpr size_t calc_size(std::string_view s){
		return s.size();
	}

	constexpr size_t calc_size(char){
		return 1;
	}

	constexpr size_t fast_digit_estimation(uint64_t const a){
		return
			(a < 10llu				)	?  1	:
			(a < 100llu				)	?  2	:
			(a < 1000llu				)	?  3	:
			(a < 10000llu				)	?  4	:

			(a < 10000'00llu			)	?  6	:

			(a < 10000'0000llu			)	?  8	:
			(a < 10000'00000'00000'00llu		)	? 16	:
			24
		;
	}
}



class RedisProtocol{
public:
	constexpr static inline char			STAR		= '*';
	constexpr static inline char			DOLLAR		= '$';
	constexpr static inline char			COLON		= ':';

	constexpr static inline std::string_view	ENDLN		= "\r\n";

public:
	constexpr static inline size_t			MAX_PARAMS	= 25;

	static_assert(MAX_PARAMS >= 7, "7 params is the minimum - pfmerge dest a b c d e");

	// originally was 4
	// setex name 100 hello
	// getx  name 100 prefix
	//
	// changed to 7
	// pfmerge dest a b c d e
	//
	// changed to 25, because redis on the web, have similar limit.
	// del a b c...

public:
	using Status = ProtocolStatus;

	#if 0
	using StringVector = std::vector<std::string_view>;
	#else
	using StringVector = StaticVector<std::string_view, MAX_PARAMS>;
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
//	template<class Buffer>
//	static void response_empty(Buffer &buffer);

	template<class Buffer>
	static void response_ok(Buffer &buffer);

	template<class Buffer>
	static void response_bool(Buffer &buffer, bool b);

	template<class Buffer>
	static void response_error(Buffer &buffer, std::string_view msg);

	template<class Buffer>
	static void response_string(Buffer &buffer, std::string_view msg);

	template<class Buffer, typename T>
	static void response_number(Buffer &buffer, T n);

	template<class Buffer>
	static void response_strings(Buffer &buffer, MySpan<const std::string_view> const &list);

private:
	template<class Buffer>
	static void response_string_nr(Buffer &buffer, std::string_view msg);

	template<class Buffer>
	static void response_string(Buffer &buffer, uint64_t const msg){
		// if we change uint64_t to template, std::string will come here.
		return response_string(buffer, std::to_string(msg));
	}

	constexpr static size_t calc_sstr(std::string_view s);

	static void response_log_(std::string_view const type, size_t const res, size_t const capacity, const void *p){
		logger<Logger::DEBUG>() << "Responce" << type << "reserve" << res << "capacity" << capacity << "addr" << p;
	}

private:
	StringVector	params_;
};


// ==================================


// template<class Buffer>
// void RedisProtocol::response_empty(Buffer &buffer){
// 	buffer.push("$-1");
// 	buffer.push(ENDLN);
// }

template<class Buffer>
void RedisProtocol::response_ok(Buffer &buffer){
	using namespace redis_protocol_impl_;

	constexpr std::string_view OK = "+OK";

	constexpr size_t res =
		calc_size(OK		)	+
		calc_size(ENDLN		)
	;

	buffer.reserve(res);

	buffer.push(OK);
	buffer.push(ENDLN);

	response_log_("ok", res, buffer.capacity(), buffer.data());
}

template<class Buffer>
void RedisProtocol::response_bool(Buffer &buffer, bool const b){
	using namespace redis_protocol_impl_;

	constexpr std::string_view TRUE  = ":1";
	constexpr std::string_view FALSE = ":0";

	static_assert(TRUE.size() == FALSE.size());

	constexpr size_t res =
		calc_size(TRUE	)	+
		calc_size(ENDLN	)
	;

	buffer.reserve(res);

	buffer.push(b ? TRUE : FALSE);
	buffer.push(ENDLN);

	response_log_("bool", res, buffer.capacity(), buffer.data());
}

template<class Buffer>
void RedisProtocol::response_error(Buffer &buffer, std::string_view const msg){
	using namespace redis_protocol_impl_;

	constexpr std::string_view ERR = "-ERR ";

	const size_t res =
		calc_size(ERR		)	+
		calc_size(msg		)	+
		calc_size(ENDLN		)
	;

	buffer.reserve(res);

	buffer.push(ERR);
	buffer.push(msg);
	buffer.push(ENDLN);

	response_log_("error", res, buffer.capacity(), buffer.data());
}

template<class Buffer>
void RedisProtocol::response_string_nr(Buffer &buffer, std::string_view const msg){
	to_string_buffer_t std_buffer;

	buffer.push(DOLLAR);
	buffer.push(to_string(msg.size(), std_buffer));
	buffer.push(ENDLN);

	buffer.push(msg);
	buffer.push(ENDLN);
}

template<class Buffer, typename T>
void RedisProtocol::response_number(Buffer &buffer, T const n){
	static_assert(
		std::is_same_v<T, uint64_t	> ||
		std::is_same_v<T, int64_t	> ||
		true
	);

	to_string_buffer_t std_buffer;

	constexpr auto res = to_string_buffer_t_size;

	buffer.reserve(res);

	buffer.push(COLON);
	buffer.push(to_string(n, std_buffer));
	buffer.push(ENDLN);

	response_log_("number", res, buffer.capacity(), buffer.data());
}

constexpr size_t RedisProtocol::calc_sstr(std::string_view s){
	using namespace redis_protocol_impl_;

	return
		calc_size(DOLLAR		)	+
		fast_digit_estimation(s.size()	)	+
		calc_size(ENDLN			)	+
		calc_size(s			)	+
		calc_size(ENDLN			)
	;
}

template<class Buffer>
void RedisProtocol::response_string(Buffer &buffer, std::string_view const msg){
	const size_t res = calc_sstr(msg);

	buffer.reserve(res);

	response_string_nr(buffer, msg);

	response_log_("string<1>", res, buffer.capacity(), buffer.data());
}

template<class Buffer>
void RedisProtocol::response_strings(Buffer &buffer, MySpan<const std::string_view> const &list){
	using namespace redis_protocol_impl_;

	const size_t res =
		calc_size(STAR				)	+
		fast_digit_estimation(list.size()	)	+
		calc_size(ENDLN				)	+

		std::accumulate(std::begin(list), std::end(list), size_t{ 0 }, [](size_t sum, std::string_view s){
			return sum + calc_sstr(s);
		})
	;

	buffer.reserve(res);

	to_string_buffer_t std_buffer;

	buffer.push(STAR);
	buffer.push(to_string(list.size(), std_buffer));
	buffer.push(ENDLN);

	for(const auto &msg : list)
		response_string_nr(buffer, msg);


	response_log_("strings<*>", res, buffer.capacity(), buffer.data());
}


} // namespace protocol
} // namespace

#endif

