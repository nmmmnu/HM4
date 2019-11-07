#ifndef LIST_DBADAPTER_H_
#define LIST_DBADAPTER_H_

#include "mystring.h"

#include <iostream>
#include <algorithm>

#include "fixedvector.h"

template<class LIST, class COMMAND=std::nullptr_t>
class ListDBAdapter{
public:
	constexpr static uint16_t DEFAULT_RESULTS = 10;
	constexpr static uint16_t MAXIMUM_RESULTS = 1000;

public:
	constexpr static bool MUTABLE = ! std::is_const<LIST>::value;

public:
	ListDBAdapter(LIST &list, COMMAND &cmd) :
				list_(list),
				cmd_(& cmd){}

	ListDBAdapter(LIST &list) :
				ListDBAdapter(list, nullptr){}

public:
	// Immutable Methods

	std::string get(std::string_view const key) const{
		assert(!key.empty());

		const auto p = list_.find(key, std::true_type{} );

		if (p != std::end(list_) && p->isValid(/* tomb */ true))
			return std::string{ p->getVal() };
		else
			return {};
	}

	auto getall(std::string_view const key, uint16_t const resultsCount, std::string_view const prefix) const{
		auto const maxResults  = std::clamp(resultsCount,  DEFAULT_RESULTS, MAXIMUM_RESULTS);

		#if 0
		std::vector<std::string_view> result;

		// reserve x2 because of hgetall
		result.reserve(maxResults * 2);
		#else

		FixedVector<std::string_view, MAXIMUM_RESULTS> result;

		#endif

		auto it = key.empty() ? std::begin(list_) : list_.find(key, std::false_type{} );

		size_t c = 0;
		for(; it != std::end(list_); ++it){
			const auto &resultKey = it->getKey();

			if (prefix.empty() == false && ! samePrefix__(prefix, resultKey))
				break;

			result.emplace_back(resultKey);

			if (it->isValid(/* tomb */ true))
				result.emplace_back(it->getVal());
			else
				result.emplace_back();

			if (++c >= maxResults)
				break;
		}

		return result;
	}

	std::string info() const{
		to_string_buffer_t buffer[2];

		return concatenate(
			"Keys (estimated): "	, to_string(list_.size(),  buffer[0]),	"\n",
			"Size: "		, to_string(list_.bytes(), buffer[1]),	"\n",
			"Mutable: "		, MUTABLE ? "Yes" : "No",		"\n"
		);
	}

	bool refresh(bool const completeRefresh){
		return refresh_(completeRefresh, cmd_);
	}

private:
	template<class T>
	bool refresh_(bool const completeRefresh, const T &){
		return cmd_ && cmd_->command(completeRefresh);
	}

	bool refresh_(bool const /* completeRefresh */, std::nullptr_t){
		return false;
	}

public:
	// Mutable Methods

	void set(std::string_view const key, std::string_view const val, std::string_view const exp = {} ){
		assert(!key.empty());

		auto const expires = from_string<uint32_t>(exp);

		list_.insert(key, val, expires);
	}

	bool del(std::string_view const key){
		assert(!key.empty());

		return list_.erase(key);
	}

	std::string incr(std::string_view const key, int64_t const val){
		assert(!key.empty());

		const auto p = list_.find(key, std::true_type{} );

		int64_t n = val;

		if (p != std::end(list_) && p->isValid(/* tomb */ true))
			n += from_string<int64_t>(p->getVal());

		std::string const s = to_string(n);

		set(key, s);

		return s;
	}

private:
	static bool samePrefix__(std::string_view const p, std::string_view const s){
		if (p.size() > s.size())
			return false;

		return std::equal(p.begin(), p.end(), s.begin());
	}

private:
	LIST		&list_;
	COMMAND		*cmd_		= nullptr;
};


#endif

