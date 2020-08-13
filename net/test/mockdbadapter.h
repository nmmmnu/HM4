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

	template<class Accumulator>
	auto foreach(std::string_view, uint16_t, std::string_view, Accumulator &accumulator) const{
		accumulator("key1", "value1");
		accumulator("key2", "value2");
		accumulator("key3", "value3");
		accumulator("key4", "value4");

		return accumulator.result();
	}

	constexpr
	static auto do_accumulate_(){
		return std::array<std::string_view, 2>{ "0", "" };
	}

	constexpr
	static auto count(std::string_view, uint16_t, std::string_view){
		return do_accumulate_();
	}

	constexpr
	static auto sum(std::string_view, uint16_t, std::string_view){
		return do_accumulate_();
	}

	constexpr
	static auto min(std::string_view, uint16_t, std::string_view){
		return do_accumulate_();
	}

	constexpr
	static auto max(std::string_view, uint16_t, std::string_view){
		return do_accumulate_();
	}

	constexpr
	static bool save(){
		return true;
	}

	constexpr
	static bool reload(){
		return true;
	}
};

#endif

