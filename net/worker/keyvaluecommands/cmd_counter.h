#include "base.h"
#include "mystring.h"



namespace net::worker{


	namespace counter_impl_{

		template<int Sign, class Protocol, class DBAdapter>
		WorkerStatus do_incr_decr(Protocol &protocol, DBAdapter &db, IOBuffer &buffer){
			const auto &p = protocol.getParams();

			if (p.size() != 2 && p.size() != 3)
				return error::BadRequest(protocol, buffer);

			const auto &key = p[1];

			if (key.empty())
				return error::BadRequest(protocol, buffer);

			int64_t n = p.size() == 3 ? Sign * from_string<int64_t>(p[2]) : Sign;

			if (n == 0)
				return error::BadRequest(protocol, buffer);

			if (! key.empty())
				n += from_string<int64_t>( db.get(key) );

			to_string_buffer_t std_buffer;

			std::string_view const val = to_string(n, std_buffer);

			db.set(key, val);

			protocol.response_string(buffer, val);

			return WorkerStatus::WRITE;
		}
	}



	template<class Protocol, class DBAdapter>
	struct cmd_INCR : cmd_base<Protocol, DBAdapter>{
		constexpr inline static std::string_view cmd[] = {
			"incr",		"INCR",
			"incrby",	"INCRBY"
		};

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) const final{
			using namespace counter_impl_;

			return do_incr_decr<+1>(protocol, db, buffer);
		}
	};



	template<class Protocol, class DBAdapter>
	struct cmd_DECR : cmd_base<Protocol, DBAdapter>{
		constexpr inline static std::string_view cmd[] = {
			"decr",		"DECR",
			"decrby",	"DECRBY"
		};

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) const final{
			using namespace counter_impl_;

			return do_incr_decr<-1>(protocol, db, buffer);
		}
	};



} // namespace

