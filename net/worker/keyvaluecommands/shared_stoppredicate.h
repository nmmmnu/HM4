#ifndef SHARED_STOP_PREDICATE_H_
#define SHARED_STOP_PREDICATE_H_

namespace net::worker::shared::stop_predicate{

	struct StopPrefixPredicate{
		std::string_view prefix;

		constexpr StopPrefixPredicate(std::string_view prefix) : prefix(prefix){
			assert(!prefix.empty());
		}

		bool operator()(std::string_view key) const{
			return ! same_prefix(prefix, key);
		}
	};

	struct StopRangePredicate{
		std::string_view end;

		constexpr StopRangePredicate(std::string_view end) : end(end){
			assert(!end.empty());
		}

		constexpr bool operator()(std::string_view key) const{
			return key > end;
		}
	};

	struct StopRangePrefixPredicate{
		std::string_view end;

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

