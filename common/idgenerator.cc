#include "idgenerator.h"

#include "mytime.h"

#include <cstdio>

namespace{
	template<size_t N>
	std::string_view idgeneratorTS(std::array<char, N> &buffer, const char *format, uint8_t serverID){
		auto const now = mytime::now();

		snprintf(buffer.data(), buffer.size(),
				format,
					mytime::to32(now),
					mytime::toUsec(now),
					serverID
		);

		return buffer.data();
	}

	template<size_t N>
	std::string_view idgeneratorDate(std::array<char, N> &buffer, const char *format, uint8_t serverID){
		constexpr auto FORMAT = mytime::TIME_FORMAT_NUMBER;

		auto const now = mytime::now();

		mytime::to_string_buffer_t time_buffer;

		snprintf(buffer.data(), buffer.size(),
				format,
					mytime::toString(now, FORMAT, time_buffer).data(),
					mytime::toUsec(now),
					serverID
		);

		return buffer.data();
	}
}

namespace idgenerator{

	std::string_view IDGeneratorTS_HEX::operator()(to_string_buffer_t &buffer) const{
		return idgeneratorTS(buffer, "%08x.%06x.%02x", serverID);
	}

	std::string_view IDGeneratorTS_HEXMono::operator()(to_string_buffer_t &buffer) const{
		return idgeneratorTS(buffer, "%08x%06x%02x", serverID);
	}

	std::string_view IDGeneratorTS_DEC::operator()(to_string_buffer_t &buffer) const{
		return idgeneratorTS(buffer, "%010u.%06x.%02x", serverID);
	}

	std::string_view IDGeneratorDate::operator()(to_string_buffer_t &buffer) const{
		return idgeneratorDate(buffer, "%s.%06x.%02x", serverID);
	}

	std::string_view IDGeneratorDateMono::operator()(to_string_buffer_t &buffer) const{
		return idgeneratorDate(buffer, "%s%06x%02x", serverID);
	}

} // namespace idgenerator

