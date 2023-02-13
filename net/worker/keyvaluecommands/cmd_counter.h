#include "base.h"



namespace net::worker::commands::Counter{

	constexpr bool MAY_RETURN_STRING = false;

	namespace counter_impl_{

		template<int Sign, class Protocol, class List>
		void do_incr_decr(ParamContainer const &p, List &list, Result<Protocol> &result){
			if (p.size() != 2 && p.size() != 3)
				return;

			const auto &key = p[1];

			if (key.empty())
				return;

			int64_t n = p.size() == 3 ? Sign * from_string<int64_t>(p[2]) : Sign;

			if (n == 0)
				return;



			if (auto const val = hm4::getPairVal(list, key); !std::empty(val)){
				n += from_string<int64_t>(val);

				if constexpr(MAY_RETURN_STRING){
					result.set(val);
				}else{
					result.set(n);
				}
			}else{
				result.set(n);
			}

			to_string_buffer_t buffer;

			auto const val = to_string(n, buffer);

			hm4::insert(list, key, val);
		}
	}



	template<class Protocol, class DBAdapter>
	struct INCR : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace counter_impl_;

			return do_incr_decr<+1>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"incr",		"INCR",
			"incrby",	"INCRBY"
		};
	};



	template<class Protocol, class DBAdapter>
	struct DECR : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace counter_impl_;

			return do_incr_decr<-1>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"decr",		"DECR",
			"decrby",	"DECRBY"
		};
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

