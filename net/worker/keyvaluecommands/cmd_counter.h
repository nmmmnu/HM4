#include "base.h"



namespace net::worker::commands::Counter{

	constexpr bool MAY_RETURN_STRING = false;

	namespace counter_impl_{

		template<int Sign, class Protocol, class DBAdapter>
		void do_incr_decr(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 2 && p.size() != 3)
				return;

			const auto &key = p[1];

			if (key.empty())
				return;

			int64_t n = p.size() == 3 ? Sign * from_string<int64_t>(p[2]) : Sign;

			if (n == 0)
				return;

			auto it = db.find(key);

			if (it && it->isValid()){
				auto const &val = it->getVal();

				n += from_string<int64_t>(val);

				if constexpr(MAY_RETURN_STRING){
					result.set(val);
				}else{
					result.set(n);
				}
			}else{
				result.set_0();
			}

			to_string_buffer_t buffer;

			auto const val = to_string(n, buffer);

			db.set(key, val);

			return;
		}
	}



	template<class Protocol, class DBAdapter>
	struct INCR : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "incr";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"incr",		"INCR",
			"incrby",	"INCRBY"
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace counter_impl_;

			return do_incr_decr<+1>(params, db, result);
		}
	};



	template<class Protocol, class DBAdapter>
	struct DECR : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "decr";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"decr",		"DECR",
			"decrby",	"DECRBY"
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace counter_impl_;

			return do_incr_decr<-1>(params, db, result);
		}
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "counters";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				INCR	,
				DECR
			>(pack);
		}
	};



} // namespace

