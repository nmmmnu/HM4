#include "base.h"

#include <limits>
#include <algorithm>



namespace net::worker::commands::Accumulators{

	namespace acumulators_impl_{

		constexpr static uint32_t MIN		= 10;
		constexpr static uint32_t ITERATIONS	= 65'536;



		template<typename Accumulator, class It>
		auto accumulateResults(uint32_t const maxResults, std::string_view const prefix, It it, It eit){
			Accumulator accumulator;

			uint32_t iterations	= 0;
			uint32_t results	= 0;

			for(;it != eit;++it){
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



		template<class Accumulator, class Protocol, class List>
		void execCommand(ParamContainer const &p, List &list, Result<Protocol> &result){
			if (p.size() != 4)
				return;



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
							count					,
							prefix					,
							list.find(key, std::false_type{})	,
							std::end(list)
			);

			to_string_buffer_t buffer;

			const std::array<std::string_view, 2> container{
				to_string(number, buffer),
				lastKey
			};

			return result.set_container(container);
		}
	} // namespace



	template<class Protocol, class DBAdapter>
	struct COUNT : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
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

			return execCommand<COUNT_>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"count",	"COUNT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SUM : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
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

			return execCommand<SUM_>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"sum",		"SUM"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MIN : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
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

			return execCommand<MIN_>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"min",		"MIN"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MAX : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
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

			return execCommand<MAX_>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"max",		"MAX"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "accumulators";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				COUNT	,
				SUM	,
				MIN	,
				MAX
			>(pack);
		}
	};



} // namespace

