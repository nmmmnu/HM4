#ifndef LIST_DBADAPTER_H_
#define LIST_DBADAPTER_H_

#include "stou_safe.h"

#include <sstream>

template<class LIST, class COMMAND=std::nullptr_t>
class ListDBAdapter{
public:
	constexpr static uint16_t DEFAULT_RESULTS = 10;
	constexpr static uint16_t MAXIMUM_RESULTS = 1000;

public:
	constexpr static bool IS_MUTABLE = ! std::is_const<LIST>::value;

public:
	ListDBAdapter(LIST &list, COMMAND &cmd) :
				list_(list),
				cmd_(& cmd){}

	ListDBAdapter(LIST &list) :
				ListDBAdapter(list, nullptr){}

public:
	// Immutable Methods

	std::string get(const StringRef &key) const{
		assert(!key.empty());

		const auto &p = list_[key];

		if (p.isValid(/* tomb */ true))
			return p.getVal();
		else
			return {};
	}

	std::vector<std::string> getall(const StringRef &key, const StringRef &maxResultsStr = {}, const StringRef &prefixCheckStr = {}) const{
		auto const maxResults  = convert__(maxResultsStr,  DEFAULT_RESULTS, MAXIMUM_RESULTS);
		auto const prefixCheck = convert__(prefixCheckStr, 0, 1);

		std::vector<std::string> result;

		// reserve x2 because of hgetall
		result.reserve(maxResults * 2);

		const auto bit = key.empty() ? list_.begin() : list_.lowerBound(key);
		const auto eit = list_.end();

		size_t c = 0;
		for(auto it = bit; it != eit; ++it){
			const auto &resultKey = it->getKey();

			if (prefixCheck && ! samePrefix__(key, resultKey))
				break;

			result.push_back(resultKey);

			if (it->isValid(/* tomb */ true))
				result.push_back(it->getVal());
			else
				result.emplace_back();

			if (++c >= maxResults)
				break;
		}

		return result;
	}

	std::string info() const{
		std::stringstream ss;

		ss	<< "Keys (estimated): "	<< list_.size(true)		<< '\n'
			<< "Size: "		<< list_.bytes()		<< '\n'
			<< "Mutable: "		<< (IS_MUTABLE ? "Yes" : "No")	<< '\n'
		;

		return ss.str();
	}

	bool refresh(){
		return cmd_ && cmd_->command();
	}

public:
	// Mutable Methods

	void set(const StringRef &key, const StringRef &val, const StringRef &exp = {} ){
		assert(!key.empty());

		auto const expires = stou_safe<uint32_t>(exp);

		list_.emplace( key, val, expires );
	}

	bool del(const StringRef &key){
		assert(!key.empty());

		return list_.erase(key);
	}

private:
	template <typename T>
	static T convert__(const StringRef &s, T const def, T const max){
		auto const result = stou_safe<T>(s, def);

		return result < max ? result : max;
	}

	static bool samePrefix__(const StringRef &p, const StringRef &s){
		if (p.size() > s.size())
			return false;

		return std::equal(p.begin(), p.end(), s.begin());

	}

private:
	LIST		&list_;
	COMMAND		*cmd_		= nullptr;
};


#endif

