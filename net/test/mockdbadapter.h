#ifndef MOCK_DB_ADAPTER_H_
#define MOCK_DB_ADAPTER_H_

#include <cassert>

#include <string_view>

#include "vectorlist.h"
#include "stdallocator.h"
#include "pmallocator.h"

struct MockDBAdapter{
	constexpr static bool MUTABLE = true;

	// Immutable Methods

	std::string_view get(std::string_view const key) const{
		auto it = list_.find(key, std::true_type{});

		return it != std::end(list_) ? it->getVal() : "";
	}

	constexpr
	static uint64_t ttl(std::string_view){
		return 0;
	}

	auto search(std::string_view const key = "") const{
		return key.empty() ? std::begin(list_) : list_.find(key, std::false_type{} );
	}

	auto end() const{
		return std::end(list_);
	}

	// System Methods

	constexpr static std::string_view info(std::string &){
		return "Mock Adapter\n";
	}

	constexpr
	static bool save(){
		return true;
	}

	constexpr
	static bool reload(){
		return true;
	}

	// Mutable Methods

	void set(std::string_view const key, std::string_view const val, uint32_t expires = 0){
		assert(!key.empty());

		list_.insert(key, val, expires);
	}

	bool del(std::string_view const key){
		assert(!key.empty());

		return list_.erase(key);
	}

private:
	using Allocator = MyAllocator::PMOwnerAllocator<MyAllocator::STDAllocator>;

	Allocator allocator_;

	hm4::VectorList<MyAllocator::PMAllocator> list_{ allocator_ };
};

#endif

