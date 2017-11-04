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
	constexpr static bool MUTABLE = ! std::is_const<LIST>::value;

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

		const auto p = list_[key];

		if (p && p->isValid(/* tomb */ true))
			return p->getVal();
		else
			return {};
	}

	std::vector<std::string> getall(const StringRef &key, uint16_t const resultsCount, bool const prefixCheck) const{
		auto const maxResults  = my_clamp__(resultsCount,  DEFAULT_RESULTS, MAXIMUM_RESULTS);

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
			<< "Mutable: "		<< (MUTABLE ? "Yes" : "No")	<< '\n'
		;

		return ss.str();
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

	void set(const StringRef &key, const StringRef &val, const StringRef &exp = {} ){
		assert(!key.empty());

		auto const expires = stou_safe<uint32_t>(exp);

		list_.insert( { key, val, expires } );
	}

	bool del(const StringRef &key){
		assert(!key.empty());

		return list_.erase(key);
	}

private:
	template<typename T>
	constexpr static T my_clamp__(T const val, T const min, T const max){
		if (val < min)
			return min;

		if (val > max)
			return max;

		return val;
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

