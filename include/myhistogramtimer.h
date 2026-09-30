#ifndef MY_HISTOGRAM_TIMER_H_
#define MY_HISTOGRAM_TIMER_H_

#include "mytime.h"

#include <cstring>
#include <numeric>	// accumulate

namespace my_histogram_timer{

	class Timer{
		constexpr static size_t NUM_BUCKETS = 28;

		std::array<uint64_t, NUM_BUCKETS> buckets{};

	public:
		constexpr void clear(){
			buckets = {};
		}

	public:
		[[nodiscard]]
		constexpr static size_t size(){
			return NUM_BUCKETS;
		}

		[[nodiscard]]
		constexpr auto begin() const{
			return std::begin(buckets);
		}

		[[nodiscard]]
		constexpr auto end() const{
			return std::end(buckets);
		}

		[[nodiscard]]
		constexpr auto const &operator[](size_t index) const{
			return buckets[index];
		}

	public:
		auto operator()(){
			struct ScopeTimer{
				ScopeTimer(Timer *timer) :
					timer		(timer		),
					start_us	(timer->start()	){}

				~ScopeTimer(){
					timer->stop(start_us);
				}

			private:
				Timer		*timer;
				uint64_t	start_us;
			};

			return ScopeTimer(this);
		}

		[[nodiscard]]
		uint64_t start() const{
			return mytime::nowSteadyMicoseconds64();
		}

		uint64_t stop(uint64_t start_us){
			auto const now_us	= mytime::nowSteadyMicoseconds64();
			auto const diff_us	= (now_us >= start_us) ? now_us - start_us : 0;

			auto const index	= calculateBucket__(diff_us);

			++buckets[index];

			return diff_us;
		}

		auto get(size_t barMaxWidth, bool utf8 = false) const{
			struct Row{
				size_t			id		;
				std::string_view	labelF		;
				std::string_view	labelT		;
				uint64_t		value		;
				double			pct		;
				double			pctCumulative	;
				size_t			bars		;
			};

			struct Result{
				size_t				total;
				std::array<Row, NUM_BUCKETS>	events;
			};


			Result result;

			result.total  = std::accumulate(begin(), end(), uint64_t{ 0 });

			if (result.total == 0){
				for(size_t i = 0; i < size(); ++i){
					result.events[i] = Row{
						i		,
						utf8 ? human[i + 0].labelUTF8 : human[i + 0].labelASCII	,
						utf8 ? human[i + 1].labelUTF8 : human[i + 1].labelASCII	,
						0		,
						0		,
						0		,
						0
					};
				}

				return result;
			}

			double   const totalF = static_cast<double>(result.total);

			uint64_t cumulative = 0;

			for(size_t i = 0; i < size(); ++i){
				auto const value = buckets[i];

				cumulative += value;

				auto const pct           = static_cast<double>(value     ) / totalF * 100;
				auto const pctCumulative = static_cast<double>(cumulative) / totalF * 100;

				size_t bars = 0;

				if (barMaxWidth > 0 && value > 0){
					bars = static_cast<size_t>(static_cast<double>(value) / totalF * static_cast<double>(barMaxWidth));
					if (bars == 0)
						bars = 1; // if zero -> single *
				}

				result.events[i] = Row{
					i		,
					utf8 ? human[i + 0].labelUTF8 : human[i + 0].labelASCII	,
					utf8 ? human[i + 1].labelUTF8 : human[i + 1].labelASCII	,
					value		,
					pct		,
					pctCumulative	,
					bars
				};
			}

			return result;
		}

	private:
		[[nodiscard]]
		constexpr static size_t calculateBucket__(uint64_t timeDiff_us){
			if (timeDiff_us == 0)
				return 0;

			size_t bucket = 64u - static_cast<size_t>( __builtin_clzll(timeDiff_us) );

			if (bucket >= NUM_BUCKETS - 1)
				return NUM_BUCKETS - 1;

			return bucket;
		}

	public:
		struct HumanString{
			std::string_view labelASCII	;
			std::string_view labelUTF8	;
		};

		constexpr static std::array<HumanString, NUM_BUCKETS + 1> human{{
			{ "    0 us", "    0 µs" },
			{ "    1 us", "    1 µs" },
			{ "    2 us", "    2 µs" },
			{ "    4 us", "    4 µs" },
			{ "    8 us", "    8 µs" },
			{ "   16 us", "   16 µs" },
			{ "   32 us", "   32 µs" },
			{ "   64 us", "   64 µs" },
			{ "  128 us", "  128 µs" },
			{ "  256 us", "  256 µs" },
			{ "  512 us", "  512 µs" },
			{ "    1 ms", "    1 ms" },
			{ "    2 ms", "    2 ms" },
			{ "    4 ms", "    4 ms" },
			{ "    8 ms", "    8 ms" },
			{ "   16 ms", "   16 ms" },
			{ "   33 ms", "   33 ms" },
			{ "   66 ms", "   66 ms" },
			{ "  131 ms", "  131 ms" },
			{ "  262 ms", "  262 ms" },
			{ "  524 ms", "  524 ms" },
			{ " 1.05  s", " 1.05  s" },
			{ " 2.10  s", " 2.10  s" },
			{ " 4.19  s", " 4.19  s" },
			{ " 8.39  s", " 8.39  s" },
			{ "16.8   s", "16.8   s" },
			{ "33.6   s", "33.6   s" },
			{ "67.1   s", "67.1   s" },
			{ "    +inf", "       ∞" }
		}};
	};

} // namespace my_histogram_timer

#endif

