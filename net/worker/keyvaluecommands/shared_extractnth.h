#ifndef SHARED_EXTRACTNTH_H_
#define SHARED_EXTRACTNTH_H_

#include <cstdint>

#include <string_view>

#include "stringtokenizer.h"

namespace net::worker::shared::extractnth{

	inline std::string_view extractNth(size_t const nth, char const separator, std::string_view const s){
		size_t count = 0;

		for (size_t i = 0; i < s.size(); ++i)
			if (s[i] == separator)
				if (++count; count == nth)
					return s.substr(i + 1);

		return "INVALID_DATA";
	}

} // namespace net::worker::shared::extractnth

#endif

