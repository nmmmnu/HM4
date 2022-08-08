#include "idgenerator.h"

#include "mytime.h"

#include <cstdio>

namespace{
	std::string_view idgeneratorTS(char *buffer, const char *format){
		auto const now = MyTime::now();

		sprintf(buffer,
				format,
					MyTime::to32(now),
					MyTime::toUsec(now)
		);

		return buffer;
	}
}

namespace hm4{
namespace idgenerator{

	std::string_view IDGeneratorTS_HEX::operator()(to_string_buffer_t &buffer) const{
		return idgeneratorTS(buffer.data(), "%08x.%08x");
	}

	std::string_view IDGeneratorTS_DEC::operator()(to_string_buffer_t &buffer) const{
		return idgeneratorTS(buffer.data(), "%010u.%010u");
	}

	std::string_view IDGeneratorDate::operator()(to_string_buffer_t &buffer) const{
		constexpr auto FORMAT = MyTime::TIME_FORMAT_NUMBER;

		auto const now = MyTime::now();

		MyTime::to_string_buffer_t time_buffer;

		sprintf(buffer.data(),
				"%s.%010u",
					MyTime::toString(now, FORMAT, time_buffer).data(),
					MyTime::toUsec(now)
		);

		return buffer.data();
	}

} // namespace idgenerator
} // namespace
