#include "base.h"
#include "mystring.h"

#include <limits>
#include <algorithm>



namespace net::worker::commands::Accumulators{

	namespace acumulators_impl_{

		constexpr static uint16_t MIN		= 10;
		constexpr static uint16_t ITERATIONS	= 10000;



		template<typename Accumulator, class It>
		auto accumulateResults(uint16_t const maxResults, std::string_view const prefix, It it, It last){
			Accumulator accumulator;

			uint16_t iterations	= 0;
			uint16_t results	= 0;

			for(; it != last; ++it){
				auto const &key = it->getKey();

				if (++iterations > ITERATIONS)
					return accumulator.result(key);

				if (! prefix.empty() && ! same_prefix(prefix, key))
					return accumulator.result();

				if (! it->isValid(std::true_type{}))
					continue;

				if (++results > maxResults)
					return accumulator.result(key);

				accumulator(key, it->getVal());
			}

			return accumulator.result();
		}



		template<class Accumulator, class Protocol, class DBAdapter>
		Result execCommand(Protocol &protocol, ParamContainer const &p, DBAdapter &db, IOBuffer &buffer){
			if (p.size() != 4)
				return Status::ERROR;

			auto const &key    = p[1];
			auto const count   = from_string<uint16_t>(p[2]);
			auto const &prefix = p[3];

			auto const result = accumulateResults<Accumulator>(
							std::clamp(count, MIN, ITERATIONS),
							prefix,
							db.search(key),
							std::end(db)
			);

			auto const [ data, lastKey ] = result;

			to_string_buffer_t std_buffer;

			protocol.response_strings(buffer, to_string(data, std_buffer), lastKey);

			return {};
		}
	}



	template<class Protocol, class DBAdapter>
	struct COUNT : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "count";
		constexpr inline static std::string_view cmd[]	= {
			"count",	"COUNT"
		};

		Result operator()(Protocol &protocol, ParamContainer const &params, DBAdapter &db, IOBuffer &buffer) const final{
			using namespace acumulators_impl_;

			using T = int64_t;

			struct COUNT_{
				T data = 0;

				auto operator()(std::string_view, std::string_view){
					++data;
				}

				auto result(std::string_view key = "") const{
					return std::make_pair(data, key);
				}
			};

			return execCommand<COUNT_>(protocol, params, db, buffer);
		}
	};



	template<class Protocol, class DBAdapter>
	struct SUM : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "sum";
		constexpr inline static std::string_view cmd[]	= {
			"sum",		"SUM"
		};

		Result operator()(Protocol &protocol, ParamContainer const &params, DBAdapter &db, IOBuffer &buffer) const final{
			using namespace acumulators_impl_;

			using T = int64_t;

			struct SUM_{
				T data = 0;

				auto operator()(std::string_view, std::string_view val){
					data += from_string<T>(val);
				}

				auto result(std::string_view key = "") const{
					return std::make_pair(data, key);
				}
			};

			return execCommand<SUM_>(protocol, params, db, buffer);
		}
	};



	template<class Protocol, class DBAdapter>
	struct MIN : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "min";
		constexpr inline static std::string_view cmd[]	= {
			"min",		"MIN"
		};

		Result operator()(Protocol &protocol, ParamContainer const &params, DBAdapter &db, IOBuffer &buffer) const final{
			using namespace acumulators_impl_;

			using T = int64_t;

			struct MIN_{
				T data = std::numeric_limits<T>::max();

				auto operator()(std::string_view, std::string_view val){
					auto x = from_string<T>(val);

					if (x < data)
						data = x;
				}

				auto result(std::string_view key = "") const{
					return std::make_pair(data, key);
				}
			};

			return execCommand<MIN_>(protocol, params, db, buffer);
		}
	};



	template<class Protocol, class DBAdapter>
	struct MAX : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "max";
		constexpr inline static std::string_view cmd[]	= {
			"max",		"MAX"
		};

		Result operator()(Protocol &protocol, ParamContainer const &params, DBAdapter &db, IOBuffer &buffer) const final{
			using namespace acumulators_impl_;

			using T = int64_t;

			struct MAX_{
				T data = std::numeric_limits<T>::min();

				auto operator()(std::string_view, std::string_view val){
					auto x = from_string<T>(val);

					if (x > data)
						data = x;
				}

				auto result(std::string_view key = "") const{
					return std::make_pair(data, key);
				}
			};

			return execCommand<MAX_>(protocol, params, db, buffer);
		}
	};



	template<class Protocol, class DBAdapter>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "accumulators";

		template<class Storage, class Map>
		static void load(Storage &s, Map &m){
			return registerCommands<Protocol, DBAdapter, Storage, Map,
				COUNT	,
				SUM	,
				MIN	,
				MAX
			>(s, m);
		}
	};



} // namespace

