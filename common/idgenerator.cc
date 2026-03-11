#include "idgenerator.h"

#include "mytime.h"

#include <cstdio>

namespace{
	template<size_t N>
	std::string_view idgeneratorTS(std::array<char, N> &buffer, const char *format){
		auto const now = mytime::now();

		snprintf(buffer.data(), buffer.size(),
				format,
					mytime::to32(now),
					mytime::toUsec(now)
		);

		return buffer.data();
	}
}

namespace idgenerator{

	std::string_view IDGeneratorTS_HEX::operator()(to_string_buffer_t &buffer) const{
		return idgeneratorTS(buffer, "%08x.%08x");
	}

	std::string_view IDGeneratorTS_DEC::operator()(to_string_buffer_t &buffer) const{
		return idgeneratorTS(buffer, "%010u.%010u");
	}

	std::string_view IDGeneratorDate::operator()(to_string_buffer_t &buffer) const{
		constexpr auto FORMAT = mytime::TIME_FORMAT_NUMBER;

		auto const now = mytime::now();

		mytime::to_string_buffer_t time_buffer;

		snprintf(buffer.data(), buffer.size(),
				"%s.%010u",
					mytime::toString(now, FORMAT, time_buffer).data(),
					mytime::toUsec(now)
		);

		return buffer.data();
	}

} // namespace idgenerator

