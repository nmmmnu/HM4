#ifndef LIST_DBADAPTER_H_
#define LIST_DBADAPTER_H_

#include <sstream>


template<class LIST, class LOADER>
class ListDBAdapter{
public:
	constexpr static size_t DEFAULT_MAX_RESULTS = 50;

public:
	constexpr static bool IS_MUTABLE = ! std::is_const<LIST>::value;

public:
	ListDBAdapter(LIST &list, LOADER &loader, size_t const maxResults = DEFAULT_MAX_RESULTS) :
				list_(& list),
				loader_(& loader),
				maxResults_(maxResults){}

	ListDBAdapter(LIST &list, size_t const maxResults = DEFAULT_MAX_RESULTS) :
				list_(& list),
				maxResults_(maxResults){}

public:
	// Immutable Methods

	std::string get(const StringRef &key) const{
		assert(list_);
		assert(!key.empty());

		const auto &p = (*list_)[key];

		if (p.isValid(/* tomb */ true))
			return p.getVal();
		else
			return {};
	}

	std::vector<std::string> getall(const StringRef &key) const{
		assert(list_);

		std::vector<std::string> result;

		// reserve x2 because of hgetall
		result.reserve(maxResults_ * 2);

		const auto bit = key.empty() ? list_->begin() : list_->lowerBound(key);
		const auto eit = list_->end();

		size_t c = 0;
		for(auto it = bit; it != eit; ++it){
			result.push_back(it->getKey());

			if (it->isValid(/* tomb */ true))
				result.push_back(it->getVal());
			else
				result.emplace_back();

			if (++c >= maxResults_)
				break;
		}

		return result;
	}

	std::string info() const{
		assert(list_);

		std::stringstream ss;

		ss	<< "Keys (estimated): "	<< list_->size(true)	<< '\n'
			<< "Size: "		<< list_->bytes()	<< '\n'
		;

		return ss.str();
	}

	bool refresh(){
		return loader_ && loader_->refresh();
	}

public:
	// Mutable Methods

	void set(const StringRef &key, const StringRef &val, const StringRef & = {} ){
		assert(list_);
		assert(!key.empty());

		list_->emplace( key, val );
	}

	bool del(const StringRef &key){
		assert(list_);
		assert(!key.empty());

		return list_->erase(key);
	}

private:
	LIST	*list_;
	LOADER	*loader_ = nullptr;
	size_t	maxResults_;
};


#endif

