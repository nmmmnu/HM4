#ifndef MOCK_DB_ADAPTER_H_
#define MOCK_DB_ADAPTER_H_


struct MockDBAdapter{
	constexpr static bool MUTABLE = false;

	// Mock commands
	std::string info() const{
		return "Mock Adapter\n";
	}

	std::string get(const StringRef &) const{
		return "value";
	}

	const std::vector<std::string> &getall(const StringRef &, uint16_t, const StringRef &) const{
		return data_;
	}

	bool refresh(bool){
		return true;
	}

private:
	std::vector<std::string> data_ = {
		"key1", "value1",
		"key2", "value2",
		"key3", "value3",
		"key4", "value4"
	};
};

#endif

