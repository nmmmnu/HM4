#include "base.h"

#include <limits>
#include <algorithm>

#include "shared_stoppredicate.h"

namespace net::worker::commands::Accumulators{
	namespace impl_{

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



		template<class Protocol, class MemoryManager>
		void execCommandSet_(Result<Protocol,MemoryManager> &result, std::string_view data, std::string_view lastKey){
			return result.set_containerN(
				data,
				lastKey
			);
		}

		template<class Protocol, class MemoryManager, class Int>
		void execCommandSet_(Result<Protocol,MemoryManager> &result, Int data, std::string_view lastKey){
			to_string_buffer_t buffer;

			return result.set_containerN(
				to_string(data, buffer),
				lastKey
			);
		}

		template<class Accumulator, class StopPredicate, class Protocol, class MemoryManager, class List>
		void execCommand_(std::string_view key, uint32_t count, std::string_view prefix, List &list, Result<Protocol,MemoryManager> &result){
			StopPredicate stop{ prefix };

			auto const [ data, lastKey ] = accumulateResults<Accumulator>(
							count		,
							stop		,
							list.find(key)	,
							std::end(list)
			);

			return execCommandSet_(result, data, lastKey);
		}

		template<class Accumulator, class StopPredicate, class Protocol, class MemoryManager, class List>
		void execCommandLimit(ParamContainer const &p, List &list, Result<Protocol,MemoryManager> &result){
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const key    = p[1];
			auto const count  = myClamp<uint32_t>(p[2], ITERATIONS_LOOPS_MIN, ITERATIONS_LOOPS_MAX);
			auto const prefix = p[3];

			if (prefix.empty())
				return result.set_error(ResultErrorMessages::EMPTY_PREFIX);

			return execCommand_<Accumulator, StopPredicate>(key, count, prefix, list, result);
		}

		template<class Accumulator, class StopPredicate, class Protocol, class MemoryManager, class List>
		void execCommand(ParamContainer const &p, List &list, Result<Protocol,MemoryManager> &result){
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

	} // namespace impl_



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct COUNT : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		COUNT() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("COUNT", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			using namespace impl_;

			return execCommandLimit<COUNTPredicate, StopPrefixPredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"count",	"COUNT"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct SUM : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		SUM() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("SUM", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			using namespace impl_;

			return execCommandLimit<SUMPredicate, StopPrefixPredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"sum",		"SUM"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XNCOUNT : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XNCOUNT() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XNCOUNT", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			using namespace impl_;

			return execCommand<COUNTPredicate, StopPrefixPredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xncount",	"XNCOUNT"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XRCOUNT : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XRCOUNT() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XRCOUNT", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			using namespace impl_;

			return execCommand<COUNTPredicate, StopRangePredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xrcount",	"XRCOUNT"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XNSUM : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XNSUM() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XNSUM", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			using namespace impl_;

			return execCommand<SUMPredicate, StopPrefixPredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xnsum",	"XNSUM"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XRSUM : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XRSUM() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XRSUM", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			using namespace impl_;

			return execCommand<SUMPredicate, StopRangePredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xrsum",	"XRSUM"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XNMIN : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XNMIN() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XNMIN", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			using namespace impl_;

			return execCommand<MINPredicate, StopPrefixPredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xnmin",	"XNMIN"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XRMIN : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XRMIN() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XRMIN", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			using namespace impl_;

			return execCommand<MINPredicate, StopRangePredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xrmin",	"XRMIN"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XNMAX : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XNMAX() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XNMAX", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			using namespace impl_;

			return execCommand<MAXPredicate, StopPrefixPredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xnmax",	"XNMAX"
		};

	};


	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XRMAX : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XRMAX() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XRMAX", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			using namespace impl_;

			return execCommand<MAXPredicate, StopRangePredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xrmax",	"XRMAX"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XNFIRST : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XNFIRST() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XNFIRST", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			using namespace impl_;

			return execCommand<FirstPredicate, StopPrefixPredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xnfirst",	"XNFIRST"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XRFIRST : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XRFIRST() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XRFIRST", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			using namespace impl_;

			return execCommand<FirstPredicate, StopRangePredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xrfirst",	"XRFIRST"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XNLAST : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XNLAST() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XNLAST", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			using namespace impl_;

			return execCommand<LastPredicate, StopPrefixPredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xnlast",	"XNLAST"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XRLAST : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XRLAST() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XRLAST", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			using namespace impl_;

			return execCommand<LastPredicate, StopRangePredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xrlast",	"XRLAST"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XNAVG : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XNAVG() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XNAVG", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			using namespace impl_;

			return execCommand<AVGPredicate, StopPrefixPredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xnavg",	"XNAVG"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct XRAVG : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		XRAVG() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("XRAVG", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			using namespace impl_;

			return execCommand<AVGPredicate, StopRangePredicate>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"xravg",	"XRAVG"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter, typename RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "accumulators";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, MemoryManager, DBAdapter, RegisterPack,
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

