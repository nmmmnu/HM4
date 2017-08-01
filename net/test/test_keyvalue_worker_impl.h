#include <type_traits>

#include "worker/keyvalueworker.h"
#include "protocol/redisprotocol.h"

struct KeyValueMockAdapter{
	constexpr static std::true_type IS_MUTABLE{};

	// Mock commands
	std::string info() const{
		return "Mock Adapter\n";
	}

	std::string get(const StringRef &) const{
		return "value";
	}

	const std::vector<std::string> &getall(const StringRef &) const{
		return data_;
	}

	bool refresh(){
		return true;
	}

	void set(const StringRef &, const StringRef &, const StringRef & = {} ){
	}

	bool del(const StringRef &){
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

using MyProtocol	= net::protocol::RedisProtocol;
using MyWorker		= net::worker::KeyValueWorker<MyProtocol, KeyValueMockAdapter>;

MyWorker myWorkerFactory(){
	static KeyValueMockAdapter adapter;

	return { adapter };
}

#include "test_selector_impl.h"

