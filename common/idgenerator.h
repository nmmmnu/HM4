#ifndef _ID_GENERATOR_TS_H
#define _ID_GENERATOR_TS_H

#include <string>
#include <string_view>

namespace hm4{
namespace idgenerator{



struct IDGeneratorTS_DEC{
	constexpr static std::size_t BUFFER_SIZE = 32;
	std::string_view operator()(char *buffer) const;
};

struct IDGeneratorTS_HEX{
	constexpr static std::size_t BUFFER_SIZE = 32;
	std::string_view operator()(char *buffer) const;
};

struct IDGeneratorDate{
	constexpr static std::size_t BUFFER_SIZE = 32;
	std::string_view operator()(char *buffer) const;
};

} // namespace idgenerator
} // namespace

#endif

