#ifndef SHARED_STOP_PREDICATE_H_
#define SHARED_STOP_PREDICATE_H_

namespace net::worker::shared::stop_predicate{

	class StopIterationPredicate{
		size_t maxIterations;
		size_t iterations	= 0;

	public:
		constexpr StopIterationPredicate(size_t maxIterations) :
						maxIterations(maxIterations){}

		constexpr bool operator()(std::string_view){
			return ++iterations >= maxIterations;
		}

		constexpr size_t getIterations() const{
			return iterations;
		}
	};

	class StopPrefixPredicate{
		std::string_view prefix;

	public:
		constexpr StopPrefixPredicate(std::string_view prefix) : prefix(prefix){
			assert(!prefix.empty());
		}

		bool operator()(std::string_view key) const{
			return ! same_prefix(prefix, key);
		}
	};

	class StopRangePredicate{
		std::string_view end;

	public:
		constexpr StopRangePredicate(std::string_view end) : end(end){
			assert(!end.empty());
		}

		constexpr bool operator()(std::string_view key) const{
			return key > end;
		}
	};

	class StopRangePrefixPredicate{
		std::string_view end;

	public:
		constexpr StopRangePrefixPredicate(std::string_view end) : end(end){
			assert(!end.empty());
		}

		// The idea is as follow:
		// end: xxx~0004~
		// key: xxx~0003~aaa	-> false
		// key: xxx~0004~aaa	-> still false
		// key: xxx~0005~aaa	-> true

		constexpr bool operator()(std::string_view key) const{
			if (key > end)
				return ! same_prefix(end, key);

			return false;
		}
	};

	struct StopUnboundPredicate{
		constexpr bool operator()(std::string_view) const{
			return false;
		}
	};

} // namespace net::worker::shared::stop_predicate

#endif

