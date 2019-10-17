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

	std::string get(std::string_view) const{
		return "value";
	}

	const auto &getall(std::string_view, uint16_t, std::string_view) const{
		return data_;
	}

	bool refresh(bool){
		return true;
	}

private:
	std::array<const char *, 4 * 2> data_ = {
		"key1", "value1",
		"key2", "value2",
		"key3", "value3",
		"key4", "value4"
	};
};

#endif

