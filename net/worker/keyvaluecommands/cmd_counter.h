#include "base.h"
#include "mystring.h"



namespace net::worker::commands::Counter{

	namespace counter_impl_{

		template<int Sign, class DBAdapter>
		Result do_incr_decr(ParamContainer const &p, DBAdapter &db){
			if (p.size() != 2 && p.size() != 3)
				return Result::error();

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			int64_t n = p.size() == 3 ? Sign * from_string<int64_t>(p[2]) : Sign;

			if (n == 0)
				return Result::error();

			if (! key.empty())
				n += from_string<int64_t>( db.get(key) );

			to_string_buffer_t std_buffer;

			std::string_view const val = to_string(n, std_buffer);

			db.set(key, val);

			return Result::ok(n);
		}
	}



	template<class DBAdapter>
	struct INCR : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "incr";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"incr",		"INCR",
			"incrby",	"INCRBY"
		};

		Result operator()(ParamContainer const &params, DBAdapter &db, OutputContainer &) const final{
			using namespace counter_impl_;

			return do_incr_decr<+1>(params, db);
		}
	};



	template<class DBAdapter>
	struct DECR : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "decr";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"decr",		"DECR",
			"decrby",	"DECRBY"
		};

		Result operator()(ParamContainer const &params, DBAdapter &db, OutputContainer &) const final{
			using namespace counter_impl_;

			return do_incr_decr<-1>(params, db);
		}
	};



	template<class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "counters";

		static void load(RegisterPack &pack){
			return registerCommands<DBAdapter, RegisterPack,
				INCR	,
				DECR
			>(pack);
		}
	};



} // namespace

