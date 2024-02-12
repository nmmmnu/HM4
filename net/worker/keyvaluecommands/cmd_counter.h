#include "base.h"

#include <functional>

namespace net::worker::commands::Counter{

	namespace counter_impl_{
		namespace{

			template<int Sign, class Protocol, class List>
			void do_incr_decr(ParamContainer const &p, List &list, Result<Protocol> &result){
				if (p.size() != 2 && p.size() != 3)
					return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_12);

				const auto &key = p[1];

				if (!hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				int64_t n = p.size() == 3 ? Sign * from_string<int64_t>(p[2]) : Sign;

				if (n == 0)
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);



				if (auto *it = hm4::getPairPtr(list, key); it){
					// Case 1: Old data exists

					int64_t const nval = from_string<int64_t>(it->getVal());

					n += nval;


					to_string_buffer_t buffer;
					auto const val = to_string(n, buffer);

					result.set_number_sv(val);

					auto *hint = it;

					hm4::insertHintF<hm4::PairFactory::Normal>(list, hint, key, val);

				}else{
					// Case 2: Old data does not exists

					to_string_buffer_t buffer;
					auto const val = to_string(n, buffer);

					result.set_number_sv(val);

					hm4::insert(list, key, val);
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

		} // namespace
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
				INCR	,
				DECR	,
				INCRTO	,
				DECRTO
			>(pack);
		}
	};



} // namespace

