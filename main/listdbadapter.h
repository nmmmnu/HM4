#ifndef LIST_DBADAPTER_H_
#define LIST_DBADAPTER_H_

#include <sstream>


template<class LIST, class COMMAND=std::nullptr_t>
class ListDBAdapter{
public:
	constexpr static uint16_t DEFAULT_MAX_RESULTS = 10;

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

	std::vector<std::string> getall(const StringRef &key, const StringRef &maxResultsStr = {}) const{
		auto const maxResults = stou__<uint16_t>(maxResultsStr, DEFAULT_MAX_RESULTS);

		std::vector<std::string> result;

		// reserve x2 because of hgetall
		result.reserve(maxResults * 2);

		const auto bit = key.empty() ? list_.begin() : list_.lowerBound(key);
		const auto eit = list_.end();

		size_t c = 0;
		for(auto it = bit; it != eit; ++it){
			result.push_back(it->getKey());

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
	void set(const StringRef &key, const StringRef &val, const StringRef &exp = {} ){
		assert(!key.empty());

		auto const expires = stou__<uint32_t>(exp);

		list_.emplace( key, val, expires );
	}

	bool del(const StringRef &key){
		assert(!key.empty());

		return list_.erase(key);
	}

private:
	// Mutable Methods
	template <typename T>
	static T stou__(const StringRef &str, T const def = 0){
		static_assert(std::is_integral<T>::value, "T must be integral type");

		if (str.empty())
			return def;

		T u = 0;
		std::istringstream ss(str);
		ss >> u;
		return u;
	}

private:
	LIST		&list_;
	COMMAND		*cmd_		= nullptr;
};


#endif

