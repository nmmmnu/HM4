#ifndef MOCK_DB_ADAPTER_H_
#define MOCK_DB_ADAPTER_H_

#include <array>
#include <string_view>

struct MockDBAdapter{
	constexpr static bool MUTABLE = false;

	// Mock commands
	std::string info() const{
		return "Mock Adapter\n";
	}

	constexpr
	std::string_view get(std::string_view) const{
		return "value";
	}

	constexpr
	const auto &getall(std::string_view, uint16_t, std::string_view) const{
		return dataKeys_;
	}

	constexpr
	std::array<std::string_view, 2> count(std::string_view, uint16_t, std::string_view) const{
		return { "0", "" };
	}

	bool refresh(bool){
		return true;
	}

private:
	constexpr static std::array<std::string_view, 4 * 2> dataKeys_ = {
		"key1", "value1",
		"key2", "value2",
		"key3", "value3",
		"key4", "value4"
	};
};

#endif

