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

	std::string_view IDGeneratorTS_DEC::operator()(char *buffer) const{
		return idgeneratorTS(buffer, "%10u.%010u");
	}

	std::string_view IDGeneratorTS_HEX::operator()(char *buffer) const{
		return idgeneratorTS(buffer, "%8u.%08x");
	}

	std::string_view IDGeneratorDate::operator()(char *buffer) const{
		constexpr const char *FORMAT = MyTime::TIME_FORMAT_NUMBER;

		auto const now = MyTime::now();

		sprintf(buffer,
				"%s.%010u",
					MyTime::toString(now, FORMAT),
					MyTime::toUsec(now)
		);

		return buffer;
	}

} // namespace idgenerator
} // namespace
