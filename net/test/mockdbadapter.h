#ifndef MOCK_DB_ADAPTER_H_
#define MOCK_DB_ADAPTER_H_


struct MockDBAdapter{
	constexpr static bool IS_MUTABLE = false;

	// Mock commands
	std::string info() const{
		return "Mock Adapter\n";
	}

	std::string get(const StringRef &) const{
		return "value";
	}

	const std::vector<std::string> &getall(const StringRef &, const StringRef & = {}) const{
		return data_;
	}

	bool refresh(){
		return true;
	}
#if 0
	void set(const StringRef &, const StringRef &, const StringRef & = {} ){
	}

	bool del(const StringRef &){
		return true;
	}
#endif
private:
	std::vector<std::string> data_ = {
		"key1", "value1",
		"key2", "value2",
		"key3", "value3",
		"key4", "value4"
	};
};


#endif

