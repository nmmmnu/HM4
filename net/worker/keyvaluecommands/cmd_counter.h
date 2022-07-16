#include "base.h"
#include "mystring.h"



namespace net::worker::commands::Counter{

	namespace counter_impl_{

		template<int Sign, class Protocol, class DBAdapter>
		Result do_incr_decr(Protocol &protocol, ParamContainer const &p, DBAdapter &db, IOBuffer &buffer){
			if (p.size() != 2 && p.size() != 3)
				return Status::ERROR;

			const auto &key = p[1];

			if (key.empty())
				return Status::ERROR;

			int64_t n = p.size() == 3 ? Sign * from_string<int64_t>(p[2]) : Sign;

			if (n == 0)
				return Status::ERROR;

			if (! key.empty())
				n += from_string<int64_t>( db.get(key) );

			to_string_buffer_t std_buffer;

			std::string_view const val = to_string(n, std_buffer);

			db.set(key, val);

			protocol.response_string(buffer, val);

			return {};
		}
	}



	template<class Protocol, class DBAdapter>
	struct INCR : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "incr";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"incr",		"INCR",
			"incrby",	"INCRBY"
		};

		Result operator()(Protocol &protocol, ParamContainer const &params, DBAdapter &db, IOBuffer &buffer) const final{
			using namespace counter_impl_;

			return do_incr_decr<+1>(protocol, params, db, buffer);
		}
	};



	template<class Protocol, class DBAdapter>
	struct DECR : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "decr";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"decr",		"DECR",
			"decrby",	"DECRBY"
		};

		Result operator()(Protocol &protocol, ParamContainer const &params, DBAdapter &db, IOBuffer &buffer) const final{
			using namespace counter_impl_;

			return do_incr_decr<-1>(protocol, params, db, buffer);
		}
	};



	template<class Protocol, class DBAdapter>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "counters";

		template<class Storage, class Map>
		static void load(Storage &s, Map &m){
			return registerCommands<Protocol, DBAdapter, Storage, Map,
				INCR	,
				DECR
			>(s, m);
		}
	};



} // namespace

