#include "base.h"
#include "fixedvector.h"
#include "mystring.h"

#include <algorithm>



namespace net{
namespace worker{

	namespace acumulator_impl_{

		constexpr static uint16_t MIN		= 10;
		constexpr static uint16_t MAX		= 1000;
		constexpr static uint16_t ITERATIONS	= 10000;



		inline bool samePrefix(std::string_view const p, std::string_view const s){
			if (p.size() > s.size())
				return false;

			return std::equal(std::begin(p), std::end(p), std::begin(s));
		}



		template<typename Accumulator, class It>
		static auto do_accumulateIteratior(Accumulator &accumulator, uint16_t const maxResults, std::string_view const prefix, It it, It last){
			uint16_t iterations	= 0;
			uint16_t results	= 0;

			for(; it != last; ++it){
				auto const &key = it->getKey();

				if (++iterations > ITERATIONS)
					return accumulator.result(key);

				if (! prefix.empty() && ! samePrefix(prefix, key))
					return accumulator.result();

				if (! it->isValid(std::true_type{}))
					continue;

				if (++results > maxResults)
					return accumulator.result(key);

				accumulator(key, it->getVal());
			}

			return accumulator.result();
		}



		template<class Protocol, class DBAdapter, class Accumulator, bool ResultIsContainer>
		WorkerStatus do_accumulate(Protocol &protocol, DBAdapter &db, IOBuffer &buffer, Accumulator &accumulator, std::bool_constant<ResultIsContainer>){
			auto const &p = protocol.getParams();

			if (p.size() != 4)
				return error::BadRequest(protocol, buffer);

			auto const &key    = p[1];
			auto const count   = from_string<uint16_t>(p[2]);
			auto const &prefix = p[3];

			auto const max = ResultIsContainer ? MAX : ITERATIONS;

			auto const result = do_accumulateIteratior(
							accumulator,
							std::clamp(count, MIN, max),
							prefix,
							db.search(key),
							std::end(db)
			);

			if constexpr(ResultIsContainer){
				protocol.response_strings(buffer, result);
			}else{
				auto const [ data, lastKey ] = result;

				to_string_buffer_t std_buffer;

				protocol.response_strings(buffer, to_string(data, std_buffer), lastKey);
			}

			return WorkerStatus::WRITE;
		}

		template<class Protocol, class DBAdapter, class Accumulator>
		WorkerStatus do_accumulate(Protocol &protocol, DBAdapter &db, IOBuffer &buffer, Accumulator &accumulator){
			return do_accumulate(protocol, db, buffer, accumulator, std::false_type{});
		}
	}



	template<class Protocol, class DBAdapter>
	struct cmd_GETX : cmd_base<Protocol, DBAdapter>{

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) final{
			using namespace acumulator_impl_;

			AccumulatorVectorNew accumulator(size);

			return do_accumulate(
					protocol, db, buffer,
					accumulator,
					std::true_type{}
			);
		}

	private:
		template<size_t Size>
		using VectorGETX = FixedVector<std::string_view,Size>;

		constexpr static size_t size = 2 * acumulator_impl_::MAX + 1;

		using MyVector = VectorGETX<size>;

		struct AccumulatorVectorNew{
			MyVector data;

			AccumulatorVectorNew(typename MyVector::size_type size){
				data.reserve(size);
			}

			auto operator()(std::string_view key, std::string_view val){
				data.emplace_back(key);
				data.emplace_back(val);
			}

			const auto &result(std::string_view key = ""){
				// emplace even empty
				data.emplace_back(key);

				return data;
			}
		};

	};



	template<class Protocol, class DBAdapter>
	struct cmd_COUNT : cmd_base<Protocol, DBAdapter>{

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) final{
			using namespace acumulator_impl_;

			using T = int64_t;

			struct{
				T data = 0;

				auto operator()(std::string_view, std::string_view){
					++data;
				}

				auto result(std::string_view key = "") const{
					return std::make_pair(data, key);
				}
			} accumulator;

			return do_accumulate(
					protocol, db, buffer,
					accumulator
			);
		}
	};



	template<class Protocol, class DBAdapter>
	struct cmd_SUM : cmd_base<Protocol, DBAdapter>{

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) final{
			using namespace acumulator_impl_;

			using T = int64_t;

			struct{
				T data = 0;

				auto operator()(std::string_view, std::string_view val){
					data += from_string<T>(val);
				}

				auto result(std::string_view key = "") const{
					return std::make_pair(data, key);
				}
			} accumulator;

			return do_accumulate(
					protocol, db, buffer,
					accumulator
			);
		}
	};



	template<class Protocol, class DBAdapter>
	struct cmd_MIN : cmd_base<Protocol, DBAdapter>{

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) final{
			using namespace acumulator_impl_;

			using T = int64_t;

			struct{
				T data = std::numeric_limits<T>::max();

				auto operator()(std::string_view, std::string_view val){
					auto x = from_string<T>(val);

					if (x < data)
						data = x;
				}

				auto result(std::string_view key = "") const{
					return std::make_pair(data, key);
				}
			} accumulator;

			return do_accumulate(
					protocol, db, buffer,
					accumulator
			);
		}
	};



	template<class Protocol, class DBAdapter>
	struct cmd_MAX : cmd_base<Protocol, DBAdapter>{

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) final{
			using namespace acumulator_impl_;

			using T = int64_t;

			struct{
				T data = std::numeric_limits<T>::max();

				auto operator()(std::string_view, std::string_view val){
					auto x = from_string<T>(val);

					if (x > data)
						data = x;
				}

				auto result(std::string_view key = "") const{
					return std::make_pair(data, key);
				}
			} accumulator;

			return do_accumulate(
					protocol, db, buffer,
					accumulator
			);
		}
	};



}
}

