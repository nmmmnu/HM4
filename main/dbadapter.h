#ifndef DBADAPTER_H_
#define DBADAPTER_H_

#include <sstream>
#include <iostream>

template<class LIST, class LOADER>
class DBAdapter{
private:
	constexpr static size_t DEFAULT_MAX_RESULTS = 50;

public:
	DBAdapter(LIST &list, LOADER &loader, size_t const maxResults = DEFAULT_MAX_RESULTS) :
				list_(list),
				loader_(loader),
				maxResults_(maxResults){}

	std::string get(const StringRef &key) const{
		const auto &p = list_[key];
		std::string s = p.getVal();
	//	std::cout << s << '\n';
		return s;
	}

	std::vector<std::string> getall(const StringRef &key) const{
		std::vector<std::string> result;

		// reserve x2 because of hgetall
		result.reserve(maxResults_ * 2);

		const auto bit = list_.lowerBound(key);
		const auto eit = list_.end();

		size_t c = 0;
		for(auto it = bit; it != eit; ++it){
			result.push_back(it->getKey());
			result.push_back(it->getVal());
		//	result.push_back(es__(it->getVal()));

			if (++c >= maxResults_)
				break;
		}

	//	for(auto x : result)
	//		std::cout << x << '\n';

		return result;
	}

	std::string info() const{
		std::stringstream ss;

		ss	<< "Keys (estimated): "	<< list_.size(true)	<< '\n'
			<< "Size: "		<< list_.bytes()	<< '\n'
		;

		return ss.str();
	}

	bool refresh() const{
		return loader_.refresh();
	}


private:
	static std::string es__(const StringRef &data){
		return data.empty() ? "0" : data;
	}

private:
	LIST	&list_;
	LOADER	&loader_;
	size_t	maxResults_;
};


#endif

