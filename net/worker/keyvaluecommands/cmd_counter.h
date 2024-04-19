#include "base.h"
#include "checkoverflow.h"

namespace net::worker::commands::Counter{

	namespace counter_impl_{

		template<int Sign, class Protocol, class List>
		void do_incr_decr(ParamContainer const &p, List &list, Result<Protocol> &result){
			if (p.size() != 2 && p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_12);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using T = int64_t;

			T const nx = p.size() == 3 ? from_string<int64_t>(p[2]) : 1;

			if constexpr(false){
				// Redis compatibility

				if (nx <= 0)
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}

			T const n = Sign * nx;



			if (auto *it = hm4::getPairPtr(list, key); it){
				// Case 1: Old data exists

				T const n1 = n;
				T const n2 = from_string<T>(it->getVal());

				T const nResult = safe_overflow::incr(n1, n2);

				to_string_buffer_t buffer;
				auto const val = to_string(nResult, buffer);

				auto *hint = it;

				hm4::insertHintF<hm4::PairFactory::Normal>(list, hint, key, val);

				result.set_number_sv(val);
			}else{
				// Case 2: Old data does not exists

				T const nResult = n;

				to_string_buffer_t buffer;
				auto const val = to_string(nResult, buffer);

				hm4::insert(list, key, val);

				result.set_number_sv(val);
			}
		}



		template<int Sign, typename T>
		[[nodiscard]]
		constexpr bool check_limit(T n, T limit){
			static_assert(Sign == -1 || Sign == +1);

			if constexpr(Sign == -1){
				return n >= limit;
			}else{
				return n <= limit;
			}
		}

		template<int Sign, class Protocol, class List>
		void do_incr_decr_limit(ParamContainer const &p, List &list, Result<Protocol> &result){
			static_assert(Sign == -1 || Sign == +1);

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using T = int64_t;

			T const nx = from_string<int64_t>(p[2]);

			if constexpr(true){
				if (nx <= 0)
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}

			T const n = Sign * nx;



			if (n == 0)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			T const limit = from_string<int64_t>(p[3]);



			if (auto *it = hm4::getPairPtr(list, key); it){
				// Case 1: Old data exists

				T const n1 = n;
				T const n2 = from_string<T>(it->getVal());

				T const nResult = safe_overflow::incr(n1, n2);

				if ( check_limit<Sign>(nResult, limit) ){
					// Case 1.1: limit OK, proceed with the update

					to_string_buffer_t buffer;
					auto const val = to_string(nResult, buffer);

					auto *hint = it;

					hm4::insertHintF<hm4::PairFactory::Normal>(list, hint, key, val);

					result.set(true);
				}else{
					// Case 1.2: limit not OK

					result.set(false);
				}
			}else{
				// Case 2: Old data does not exists, assuming 0

				T const nResult = n;

				if ( check_limit<Sign>(nResult, limit) ){
					// Case 2.1: limit OK, proceed with the update

					to_string_buffer_t buffer;
					auto const val = to_string(nResult, buffer);

					hm4::insert(list, key, val);

					result.set(true);
				}else{
					// Case 2.2: limit not OK

					result.set(false);
				}
			}
		}



		template<class Protocol, class List, class Comp>
		void do_incr_to(ParamContainer const &p, List &list, Result<Protocol> &result, Comp comp){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			int64_t const n = from_string<int64_t>(p[2]);



			if (auto *it = hm4::getPairPtr(list, key); it){
				// Case 1: Old data exists

				auto const val = it->getVal();

				if (int64_t const nval = from_string<int64_t>(val); comp(n, nval)){
					// Case 1.1: Comp is satisfied, so update

					to_string_buffer_t buffer;
					auto const val = to_string(n, buffer);

					result.set_number_sv(val);

					auto *hint = it;

					hm4::insertHintF<hm4::PairFactory::Normal>(list, hint, key, val);
				}else{
					// Case 1.2: no need to update

					result.set_number_sv(val);
				}
			}else{
				// Case 2: Old data does not exists

				to_string_buffer_t buffer;
				auto const val = to_string(n, buffer);

				result.set_number_sv(val);

				hm4::insert(list, key, val);
			}
		}

	} // namespace counter_impl_



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



	template<class Protocol, class DBAdapter>
	struct INCRLIMIT : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace counter_impl_;

			return do_incr_decr_limit<+1>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"incrlimit",	"INCRLIMIT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct DECRLIMIT : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace counter_impl_;

			return do_incr_decr_limit<-1>(params, *db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"decrlimit",	"DECRLIMIT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct INCRTO : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace counter_impl_;

			return do_incr_to(params, *db, result, std::greater{});
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"incrto",	"INCRTO"
		};
	};



	template<class Protocol, class DBAdapter>
	struct DECRTO : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &params, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace counter_impl_;

			return do_incr_to(params, *db, result, std::less{});
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"decrto",	"DECRTO"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "counters";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				INCR		,
				DECR		,
				INCRLIMIT	,
				DECRLIMIT	,
				INCRTO		,
				DECRTO
			>(pack);
		}
	};



} // namespace

