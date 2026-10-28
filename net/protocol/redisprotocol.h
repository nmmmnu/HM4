#ifndef _REDIS_PROTOCOL_H
#define _REDIS_PROTOCOL_H

#include "protocoldefs.h"

#include <numeric>	// std::accumulate
#include <utility>	// std::pair
#include <string_view>

#include "mystring.h"

#include "staticvector.h"
#include "myspan.h"

#include "logger.h"



namespace net{
namespace protocol{



class RedisProtocol{
public:
	constexpr static inline char			STAR		= '*';
	constexpr static inline char			DOLLAR		= '$';
	constexpr static inline char			COLON		= ':';
	constexpr static inline char			PLUS		= '+';

	constexpr static inline std::string_view	ENDLN		= "\r\n";

public:
	constexpr static inline size_t			MAX_PARAMS	= 0xFF;

	static_assert(MAX_PARAMS >= 7, "7 params is the minimum - pfmerge dest a b c d e");

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
	class Response;

private:
	StringVector	params_;
};



// ==================================



namespace redis_protocol_impl_{
	constexpr size_t calc_size(std::string_view s){
		return s.size();
	}

	constexpr size_t calc_size(char){
		return 1;
	}

	constexpr size_t calc_size_int(uint64_t){
		return to_string_buffer_t_size;
	}

	constexpr size_t calc_size_int(int64_t){
		return to_string_buffer_t_size;
	}

	constexpr size_t calc_size_str(std::string_view s){
		constexpr char			DOLLAR	= '$';
		constexpr std::string_view	ENDLN	= "\r\n";

		return
			calc_size(DOLLAR	)	+
			calc_size_int(s.size()	)	+
			calc_size(ENDLN		)	+
			calc_size(s		)	+
			calc_size(ENDLN		)
		;
	}



	#if 0
	inline void response_log(std::string_view const type, size_t const res, size_t const capacity, const void *p){
		logger<Logger::DEBUG>() << "Responce" << type << "size" << res << "capacity" << capacity << "addr" << p;
	}

	template<class Buffer>
	void response_log(std::string_view const type, Buffer const &buffer){
		return response_log(type, buffer.size(), buffer.capacity(), buffer.data());
	}
	#endif



	inline void copy(char *&ptr, std::string_view s, size_t &size){
		memcpy(ptr, s.data(), s.size());
		ptr  += s.size();
		size += s.size();
	}

	inline void copy(char *&ptr, char c, size_t &size){
		memcpy(ptr, &c, 1);
		ptr  += 1;
		size += 1;
	}

} // namespace redis_protocol_impl_



class RedisProtocol::Response{
	static size_t responceNumberSV__(char *&ptr, std::string_view n) noexcept{
		using namespace redis_protocol_impl_;

		size_t size = 0;

		copy(ptr, COLON	, size);
		copy(ptr, n	, size);
		copy(ptr, ENDLN	, size);

		return size;
	}

	static size_t responceStringNR__(char *&ptr, std::string_view const msg) noexcept{
		using namespace redis_protocol_impl_;

		size_t size = 0;

		to_string_buffer_t std_buffer;

		auto const n = to_string(msg.size(), std_buffer);

		copy(ptr, DOLLAR	, size);
		copy(ptr, n		, size);
		copy(ptr, ENDLN		, size);

		copy(ptr, msg		, size);
		copy(ptr, ENDLN		, size);

		return size;
	}

public:
	class OK{
		constexpr static std::string_view name	= "ok";

		constexpr static std::string_view OK_	= "+OK";

	public:
		constexpr OK() = default;

		constexpr static size_t size() noexcept{
			using namespace redis_protocol_impl_;

			return	calc_size(OK_		) +
				calc_size(ENDLN		)
			;
		}

		size_t operator()(char *&ptr) const noexcept{
			using namespace redis_protocol_impl_;

			size_t size = 0;

			copy(ptr, OK_		, size);
			copy(ptr, ENDLN		, size);

			return size;
		}
	};



	class Bool{
		constexpr static std::string_view name	= "bool";

		constexpr static std::string_view TRUE	= ":1";
		constexpr static std::string_view FALSE	= ":0";

		static_assert(TRUE.size() == FALSE.size());

	public:
		constexpr Bool(bool bb) noexcept : bb(bb){}

		constexpr static size_t size() noexcept{
			using namespace redis_protocol_impl_;

			return 	calc_size(TRUE	) +
				calc_size(ENDLN	)
			;
		}

		size_t operator()(char *&ptr) const noexcept{
			using namespace redis_protocol_impl_;

			size_t size = 0;

			copy(ptr, bb ? TRUE : FALSE	, size);
			copy(ptr, ENDLN			, size);

			return size;
		}

	private:
		bool bb;
	};



	class Error{
		constexpr static std::string_view name	= "error";

		constexpr static std::string_view ERR	= "-ERR ";

	public:
		constexpr Error(std::string_view msg) noexcept : msg(msg){}

		constexpr size_t size() const noexcept{
			using namespace redis_protocol_impl_;

			return	calc_size(ERR	) +
				calc_size(msg	) +
				calc_size(ENDLN	)
			;
		}

		size_t operator()(char *&ptr) const noexcept{
			using namespace redis_protocol_impl_;

			size_t size = 0;

			copy(ptr, ERR	, size);
			copy(ptr, msg	, size);
			copy(ptr, ENDLN	, size);

			return size;
		}

	private:
		std::string_view msg;
	};



	class String{
		constexpr static std::string_view name	= "string<1>";

	public:
		constexpr String(std::string_view msg) noexcept : msg(msg){}

		constexpr size_t size() const noexcept{
			using namespace redis_protocol_impl_;

			return calc_size_str(msg);
		}

		size_t operator()(char *&ptr) const noexcept{
			using namespace redis_protocol_impl_;

			return responceStringNR__(ptr, msg);
		}

	private:
		std::string_view msg;
	};



	class SimpleString{
		constexpr static std::string_view name	= "simple_string";

	public:
		constexpr SimpleString(std::string_view msg) noexcept : msg(msg){}

		constexpr size_t size() const noexcept{
			using namespace redis_protocol_impl_;

			return 	calc_size	(PLUS	) +
				msg.size	(	) +
				calc_size	(ENDLN	)
			;
		}

		size_t operator()(char *&ptr) const noexcept{
			using namespace redis_protocol_impl_;

			size_t size = 0;

			copy(ptr, PLUS	, size);
			copy(ptr, msg	, size);
			copy(ptr, ENDLN	, size);

			return size;
		}

	private:
		std::string_view msg;
	};



	class Number64{
		constexpr static std::string_view name	= "number";

	public:
		constexpr Number64(int64_t n) noexcept : n(n){}

		constexpr size_t size() const noexcept{
			using namespace redis_protocol_impl_;

			return 	calc_size	(COLON	) +
				calc_size_int	(n	) +
				calc_size	(ENDLN	)
			;
		}

		size_t operator()(char *&ptr) const noexcept{
			using namespace redis_protocol_impl_;

			to_string_buffer_t std_buffer;

			return responceNumberSV__(ptr, to_string(n, std_buffer));
		}

	private:
		int64_t n;
	};



	class NumberU64{
		constexpr static std::string_view name	= "number";

	public:
		constexpr NumberU64(uint64_t n) noexcept : n(n){}

		constexpr size_t size() const noexcept{
			using namespace redis_protocol_impl_;

			if (n > 0x0FFF'FFFF'FFFF'FFFF){
				std::string_view const MAX64 = "18446744073709551615";

				return calc_size_str(MAX64);
			}else{
				return 	calc_size	(COLON	) +
					calc_size_int	(n	) +
					calc_size	(ENDLN	)
				;
			}
		}

		size_t operator()(char *&ptr) const noexcept{
			using namespace redis_protocol_impl_;

			to_string_buffer_t std_buffer;

			if (n > 0x0FFF'FFFF'FFFF'FFFF)
				return responceStringNR__(ptr, to_string(n, std_buffer));
			else
				return responceNumberSV__(ptr, to_string(n, std_buffer));
		}

	private:
		uint64_t n;
	};



	class NumberSV{
		constexpr static std::string_view name	= "number";

	public:
		constexpr NumberSV(std::string_view n) noexcept : n(n){}

		constexpr size_t size() const noexcept{
			using namespace redis_protocol_impl_;

			return	calc_size(COLON	) +
				calc_size(n	) +
				calc_size(ENDLN	)
			;
		}

		size_t operator()(char *&ptr) const noexcept{
			using namespace redis_protocol_impl_;

			return responceNumberSV__(ptr, n);
		}

	private:
		std::string_view n;
	};



	class Strings{
		constexpr static std::string_view name	= "string<*>";

	public:
		constexpr Strings(MySpan<const std::string_view> const &list) noexcept : list(list){}

		constexpr size_t size() const noexcept{
			using namespace redis_protocol_impl_;

			return	calc_size(STAR			)	+
				calc_size_int(list.size()	)	+
				calc_size(ENDLN			)	+

				std::accumulate(std::begin(list), std::end(list), size_t{ 0 }, [](size_t sum, std::string_view s){
					return sum + calc_size_str(s);
				})
			;
		}

		size_t operator()(char *&ptr) const noexcept{
			using namespace redis_protocol_impl_;

			to_string_buffer_t std_buffer;

			size_t size = 0;

			auto const n = to_string(list.size(), std_buffer);

			copy(ptr, STAR	, size);
			copy(ptr, n	, size);
			copy(ptr, ENDLN	, size);

			for(const auto &msg : list)
				size += responceStringNR__(ptr, msg);

			return size;
		}

	private:
		MySpan<const std::string_view>  const &list;
	};

};



} // namespace protocol
} // namespace

#endif

