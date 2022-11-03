#include "base.h"

#include <limits>
#include <algorithm>



namespace net::worker::commands::Accumulators{

	namespace acumulators_impl_{

		constexpr static uint32_t MIN		= 10;
		constexpr static uint32_t ITERATIONS	= 65'536;



		template<typename Accumulator, class It>
		auto accumulateResults(uint32_t const maxResults, std::string_view const prefix, It it){
			Accumulator accumulator;

			uint32_t iterations	= 0;
			uint32_t results	= 0;

			for(;it;++it){
				auto const key = it->getKey();

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



		template<class Accumulator, class DBAdapter>
		Result execCommand(ParamContainer const &p, DBAdapter &db, OutputBlob &blob){
			if (p.size() != 4)
				return Result::error();



			// using uint64_t from the user, allow more user-friendly behavour.
			// suppose he enters 1'000'000'000.
			// because this value is great than max uint32_t,
			// the converted value will go to 0, then to MIN.

			auto myClamp = [](auto a){
				return static_cast<uint32_t>(
					std::clamp<uint64_t>(a, MIN, ITERATIONS)
				);
			};



			auto const &key    = p[1];
			auto const count   = myClamp( from_string<uint64_t>(p[2]) );
			auto const &prefix = p[3];

			auto const [ number, lastKey ] = accumulateResults<Accumulator>(
							count				,
							prefix				,
							db.find(key, std::false_type{})
			);

			blob.container.clear();
			blob.container.push_back(
				to_string(number, blob.buffer_key)
			);
			blob.container.push_back(
				lastKey
			);

			return Result::ok_container(blob.container);
		}
	} // namespace



	template<class DBAdapter>
	struct COUNT : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "count";
		constexpr inline static std::string_view cmd[]	= {
			"count",	"COUNT"
		};

		Result operator()(ParamContainer const &params, DBAdapter &db, OutputBlob &blob) const final{
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

			return execCommand<COUNT_>(params, db, blob);
		}
	};



	template<class DBAdapter>
	struct SUM : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "sum";
		constexpr inline static std::string_view cmd[]	= {
			"sum",		"SUM"
		};

		Result operator()(ParamContainer const &params, DBAdapter &db, OutputBlob &blob) const final{
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

			return execCommand<SUM_>(params, db, blob);
		}
	};



	template<class DBAdapter>
	struct MIN : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "min";
		constexpr inline static std::string_view cmd[]	= {
			"min",		"MIN"
		};

		Result operator()(ParamContainer const &params, DBAdapter &db, OutputBlob &blob) const final{
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

			return execCommand<MIN_>(params, db, blob);
		}
	};



	template<class DBAdapter>
	struct MAX : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "max";
		constexpr inline static std::string_view cmd[]	= {
			"max",		"MAX"
		};

		Result operator()(ParamContainer const &params, DBAdapter &db, OutputBlob &blob) const final{
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

			return execCommand<MAX_>(params, db, blob);
		}
	};



	template<class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "accumulators";

		static void load(RegisterPack &pack){
			return registerCommands<DBAdapter, RegisterPack,
				COUNT	,
				SUM	,
				MIN	,
				MAX
			>(pack);
		}
	};



} // namespace

