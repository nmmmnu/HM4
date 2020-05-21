#ifndef MOCK_DB_ADAPTER_H_
#define MOCK_DB_ADAPTER_H_

#include <array>
#include <string_view>

struct MockDBAdapter{
	constexpr static bool MUTABLE = false;

	// Mock commands
	constexpr
	static std::string_view info(){
		return "Mock Adapter\n";
	}

	constexpr
	static std::string_view get(std::string_view){
		return "value";
	}

	constexpr
	static auto getall(std::string_view, uint16_t, std::string_view){
		return std::array<std::string_view, 4 * 2>{
			"key1", "value1",
			"key2", "value2",
			"key3", "value3",
			"key4", "value4"
		};
	}

	constexpr
	static auto getx(std::string_view, uint16_t, std::string_view){
		return std::array<std::string_view, 4 * 2 + 1>{
			"key1", "value1",
			"key2", "value2",
			"key3", "value3",
			"key4", "value4",
			""
		};
	}

	constexpr
	static auto count(std::string_view, uint16_t, std::string_view){
		return std::array<std::string_view, 2>{ "0", "" };
	}

	constexpr
	static auto sum(std::string_view, uint16_t, std::string_view){
		return std::array<std::string_view, 2>{ "0", "" };
	}

	constexpr
	static bool refresh(bool){
		return true;
	}
};

#endif

