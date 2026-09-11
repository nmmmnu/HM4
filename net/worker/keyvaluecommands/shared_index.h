#ifndef SHARED_INDEX_H_
#define SHARED_INDEX_H_

#include "base.h"

namespace net::worker::shared::config{

	constexpr uint32_t INDEX_TOKEN_SIZE = 104;
	// Sphinx => 40 + truncate
	// MySQL  => 84 + ignore

	// keyN + keySub + 5 indexes + keySort = 8; 8 * 104 = 832

} // namespace net::worker::shared::config

namespace net::worker::shared::index_token{

	constexpr uint32_t INDEX_TOKEN_SIZE = accumulate_results::INDEX_TOKEN_SIZE;

	constexpr bool valid(std::string_view s){
		return !s.empty() && s.size() <= INDEX_TOKEN_SIZE;
	}

	template <typename ...Ts>
	constexpr bool valid(std::string_view sv, Ts ...ts) {
		static_assert((std::is_convertible_v<Ts, std::string_view> && ...),
			"All arguments must be convertible to std::string_view");

		return (valid(sv) && ... && valid(ts));
	}

	template <size_t N>
	constexpr bool valid(std::array<std::string_view, N> const &indexes){
		if constexpr (N == 0){
			return true;
		}

		auto f = [](auto const &...items){
			return valid(items...);
		};

		return std::apply(f, indexes);
	}

	// ---------------

	constexpr bool validSize(std::string_view s){
		return s.size() <= INDEX_TOKEN_SIZE;
	}

} // namespace net::worker::shared::index_token


#endif

