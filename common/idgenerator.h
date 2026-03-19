#ifndef _ID_GENERATOR_TS_H
#define _ID_GENERATOR_TS_H

#include <array>
#include <string_view>
#include <cstdint>

namespace idgenerator{

	struct IDGeneratorTS_HEX{
		IDGeneratorTS_HEX(uint8_t serverID = 0) : serverID(serverID){}

		// 62ec3590.07c22b.SS
		// size 18
		constexpr static std::size_t to_string_buffer_t_size = 24;
		using to_string_buffer_t = std::array<char, to_string_buffer_t_size>;

		std::string_view operator()(to_string_buffer_t &buffer) const;

	private:
		uint8_t serverID;
	};

	struct IDGeneratorTS_HEXMono{
		IDGeneratorTS_HEXMono(uint8_t serverID = 0) : serverID(serverID){}

		// 62ec359007c22bSS
		// size 16
		constexpr static std::size_t to_string_buffer_t_size = 24;
		using to_string_buffer_t = std::array<char, to_string_buffer_t_size>;

		std::string_view operator()(to_string_buffer_t &buffer) const;

	private:
		uint8_t serverID;
	};

	struct IDGeneratorTS_DEC{
		IDGeneratorTS_DEC(uint8_t serverID = 0) : serverID(serverID){}

		// 1659647210.07c22b.SS
		// size 20
		constexpr static std::size_t to_string_buffer_t_size = 24;
		using to_string_buffer_t = std::array<char, to_string_buffer_t_size>;

		std::string_view operator()(to_string_buffer_t &buffer) const;

	private:
		uint8_t serverID;
	};

	struct IDGeneratorDate{
		IDGeneratorDate(uint8_t serverID = 0) : serverID(serverID){}

		// 20220805.000650.07c22b.SS
		// size 25
		constexpr static std::size_t to_string_buffer_t_size = 32;
		using to_string_buffer_t = std::array<char, to_string_buffer_t_size>;

		std::string_view operator()(to_string_buffer_t &buffer) const;

	private:
		uint8_t serverID;
	};

	struct IDGeneratorDateMono{
		IDGeneratorDateMono(uint8_t serverID = 0) : serverID(serverID){}

		// 2022080500065007c22bSS
		// size 22
		constexpr static std::size_t to_string_buffer_t_size = 24;
		using to_string_buffer_t = std::array<char, to_string_buffer_t_size>;

		std::string_view operator()(to_string_buffer_t &buffer) const;

	private:
		uint8_t serverID;
	};

} // namespace idgenerator

#endif

