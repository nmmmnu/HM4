#ifndef _ID_GENERATOR_TS_H
#define _ID_GENERATOR_TS_H

#include <array>
#include <string_view>

namespace hm4{
namespace idgenerator{



struct IDGeneratorTS_HEX{
	// 62ec3590.0007c22b
	// size 17
	constexpr static std::size_t to_string_buffer_t_size = 24;
	using to_string_buffer_t = std::array<char, to_string_buffer_t_size>;

	std::string_view operator()(to_string_buffer_t &buffer) const;
};

struct IDGeneratorTS_DEC{
	// 1659647210.00011dc9
	// size 21
	constexpr static std::size_t to_string_buffer_t_size = 24;
	using to_string_buffer_t = std::array<char, to_string_buffer_t_size>;

	std::string_view operator()(to_string_buffer_t &buffer) const;
};

struct IDGeneratorDate{
	// 20220805.000650.0000073507
	// size 26
	constexpr static std::size_t to_string_buffer_t_size = 32;
	using to_string_buffer_t = std::array<char, to_string_buffer_t_size>;

	std::string_view operator()(to_string_buffer_t &buffer) const;
};

} // namespace idgenerator
} // namespace

#endif

