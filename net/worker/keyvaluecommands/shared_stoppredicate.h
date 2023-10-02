#ifndef SHARED_STOP_PREDICATE_H_
#define SHARED_STOP_PREDICATE_H_

namespace net::worker::shared::stop_predicate{

	// making it class, makes later code prettier.
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
			return end < key;
		}
	};

	struct StopUnboundPredicate{
		constexpr bool operator()(std::string_view) const{
			return false;
		}
	};

} // namespace net::worker::shared::stop_predicate

#endif

