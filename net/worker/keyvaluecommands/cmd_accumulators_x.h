#include "base.h"

#include <limits>
#include <algorithm>

#include "shared_stoppredicate.h"

namespace net::worker::commands::Accumulators{
	namespace acumulators_impl_{

		using namespace net::worker::shared::stop_predicate;
		using namespace net::worker::shared::config;

		template<class Accumulator, class It, class StopPredicate>
		auto accumulateResults(uint32_t const maxResults, StopPredicate stop, It it, It eit){
			Accumulator accumulator;

			uint32_t iterations	= 0;
			uint32_t results	= 0;

			for(;it != eit;++it){
				auto const key = it->getKey();

				if (++iterations > ITERATIONS_LOOPS_MAX)
					return accumulator.result(key);

				if (stop(key))
					return accumulator.result();

				if (! it->isOK())
					continue;

				if (++results > maxResults)
					return accumulator.result(key);

				accumulator(it->getVal());

				if constexpr(Accumulator::ONCE)
					return accumulator.result();
			}

			return accumulator.result();
		}



		template<class Protocol>
		void execCommandSet_(Result<Protocol> &result, std::string_view data, std::string_view lastKey){
			return result.set_dual(
				data,
				lastKey
			);
		}

		template<class Protocol, class Int>
		void execCommandSet_(Result<Protocol> &result, Int data, std::string_view lastKey){
			to_string_buffer_t buffer;

			return result.set_dual(
				to_string(data, buffer),
				lastKey
			);
		}

		template<class Accumulator, class StopPredicate, class Protocol, class List>
		void execCommand_(std::string_view key, uint32_t count, std::string_view prefix, List &list, Result<Protocol> &result){
			StopPredicate stop{ prefix };

			auto const [ data, lastKey ] = accumulateResults<Accumulator>(
							count		,
							stop		,
							list.find(key)	,
							std::end(list)
			);

			return execCommandSet_(result, data, lastKey);
		}

		template<class Accumulator, class StopPredicate, class Protocol, class List>
		void execCommandLimit(ParamContainer const &p, List &list, Result<Protocol> &result){
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const key    = p[1];
			auto const count  = myClamp<uint32_t>(p[2], ITERATIONS_LOOPS_MIN, ITERATIONS_LOOPS_MAX);
			auto const prefix = p[3];

			if (prefix.empty())
				return result.set_error(ResultErrorMessages::EMPTY_PREFIX);

			return execCommand_<Accumulator, StopPredicate>(key, count, prefix, list, result);
		}

		template<class Accumulator, class StopPredicate, class Protocol, class List>
		void execCommand(ParamContainer const &p, List &list, Result<Protocol> &result){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const key    = p[1];
			auto const prefix = p[2];

			if (prefix.empty())
				return result.set_error(ResultErrorMessages::EMPTY_PREFIX);

			return execCommand_<Accumulator, StopPredicate>(key, ITERATIONS_LOOPS_MAX, prefix, list, result);
		}



		struct COUNTPredicate{
			constexpr static bool ONCE = false;

			using T = int64_t;

			T data = 0;

			auto operator()(std::string_view){
				++data;
			}

			auto result(std::string_view key = "") const{
				return std::make_pair(data, key);
			}
		};

		struct SUMPredicate{
			constexpr static bool ONCE = false;

			using T = int64_t;

			T data = 0;

			auto operator()(std::string_view val){
				data += from_string<T>(val);
			}

			auto result(std::string_view key = "") const{
				return std::make_pair(data, key);
			}
		};

		struct MINPredicate{
			constexpr static bool ONCE = false;

			using T = int64_t;

			T data = std::numeric_limits<T>::max();

			auto operator()(std::string_view val){
				auto x = from_string<T>(val);

				if (x < data)
					data = x;
			}

			auto result(std::string_view key = "") const{
				return std::make_pair(data, key);
			}
		};

		struct MAXPredicate{
			constexpr static bool ONCE = false;

			using T = int64_t;

			T data = std::numeric_limits<T>::min();

			auto operator()(std::string_view val){
				auto x = from_string<T>(val);

				if (x > data)
					data = x;
			}

			auto result(std::string_view key = "") const{
				return std::make_pair(data, key);
			}
		};

		template<bool B>
		struct FirstLastPredicate_{
			constexpr static bool ONCE = B;

			using T = std::string_view;

			T data;

			auto operator()(std::string_view val){
				data = val;
			}

			auto result(std::string_view key = "") const{
				return std::make_pair(data, key);
			}
		};

		using FirstPredicate = FirstLastPredicate_<1>;
		using LastPredicate = FirstLastPredicate_<0>;

		struct AVGPredicate{
			constexpr static bool ONCE = false;

			using T = int64_t;

			T sum   = 0;
			T count = 0;

			auto operator()(std::string_view val){
				sum += from_string<T>(val);
				++count;
			}

			auto result(std::string_view key = "") const{
				return std::make_pair(sum / count, key);
			}
		};

	} // namespace acumulators_impl_



	template<class Protocol, class DBAdapter>
	struct COUNT : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace acumulators_impl_;

			return execCommandLimit<COUNTPredicate, StopPrefixPredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"count",	"COUNT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SUM : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace acumulators_impl_;

			return execCommandLimit<SUMPredicate, StopPrefixPredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"sum",		"SUM"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XNCOUNT : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace acumulators_impl_;

			return execCommand<COUNTPredicate, StopPrefixPredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xncount",	"XNCOUNT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XRCOUNT : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace acumulators_impl_;

			return execCommand<COUNTPredicate, StopRangePredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xrcount",	"XRCOUNT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XNSUM : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace acumulators_impl_;

			return execCommand<SUMPredicate, StopPrefixPredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xnsum",	"XNSUM"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XRSUM : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace acumulators_impl_;

			return execCommand<SUMPredicate, StopRangePredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xrsum",	"XRSUM"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XNMIN : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace acumulators_impl_;

			return execCommand<MINPredicate, StopPrefixPredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xnmin",	"XNMIN"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XRMIN : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace acumulators_impl_;

			return execCommand<MINPredicate, StopRangePredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xrmin",	"XRMIN"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XNMAX : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace acumulators_impl_;

			return execCommand<MAXPredicate, StopPrefixPredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xnmax",	"XNMAX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XRMAX : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace acumulators_impl_;

			return execCommand<MAXPredicate, StopRangePredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xrmax",	"XRMAX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XNFIRST : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace acumulators_impl_;

			return execCommand<FirstPredicate, StopPrefixPredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xnfirst",	"XNFIRST"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XRFIRST : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace acumulators_impl_;

			return execCommand<FirstPredicate, StopRangePredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xrfirst",	"XRFIRST"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XNLAST : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace acumulators_impl_;

			return execCommand<LastPredicate, StopPrefixPredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xnlast",	"XNLAST"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XRLAST : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace acumulators_impl_;

			return execCommand<LastPredicate, StopRangePredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xrlast",	"XRLAST"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XNAVG : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace acumulators_impl_;

			return execCommand<AVGPredicate, StopPrefixPredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xnavg",	"XNAVG"
		};
	};



	template<class Protocol, class DBAdapter>
	struct XRAVG : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace acumulators_impl_;

			return execCommand<AVGPredicate, StopRangePredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"xravg",	"XRAVG"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "accumulators";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				COUNT	,
				SUM	,

				XNCOUNT	,
				XRCOUNT	,

				XNSUM	,
				XRSUM	,

				XNMIN	,
				XRMIN	,

				XNMAX	,
				XRMAX	,

				XNFIRST	,
				XRFIRST	,

				XNLAST	,
				XRLAST	,

				XNAVG	,
				XRAVG
			>(pack);
		}
	};



} // namespace

