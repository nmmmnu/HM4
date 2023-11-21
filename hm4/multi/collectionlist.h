#ifndef COLLECTION_LIST_H_
#define COLLECTION_LIST_H_

#include "ilist.h"

#include "collectioniterator.h"

#include <numeric>	// std::accumulate

namespace hm4{
namespace multi{


template <class StoreContainer, class Projection = std::nullptr_t>
class CollectionList{
public:
	using List		= typename StoreContainer::value_type;

	using size_type		= typename List::size_type;
	using difference_type	= typename List::difference_type;

	using iterator		= CollectionIterator<typename List::iterator, Projection>;

	using estimated_size	= std::true_type;

public:
	CollectionList(const StoreContainer &list) : list_	(&list){}

	iterator begin() const{
		return { std::begin(*list_), std::end(*list_), std::true_type{} };
	}

	iterator end() const{
		return { std::begin(*list_), std::end(*list_), std::false_type{} };
	}

	template<bool B>
	iterator find(std::string_view const key, std::bool_constant<B> const exact) const{
		return { std::begin(*list_), std::end(*list_), key, exact };
	}

public:
	size_type size() const{
		auto sum = [](size_t const result, List const &list){
			return result + list.size();
		};

		return std::accumulate(std::begin(*list_), std::end(*list_), size_t{ 0 }, sum);
	}

	auto empty() const{
		auto f = [](List const &list){
			return !list.empty();
		};

		return std::find_if(std::begin(*list_), std::end(*list_), f) == std::end(*list_);
	}

	size_t bytes() const{
		auto sum = [](size_type const result, List const &list){
			return result + list.bytes();
		};

		return std::accumulate(std::begin(*list_), std::end(*list_), size_type{ 0 }, sum);
	}

	void crontab() const{
		for(auto const &list : *list_)
			list.crontab();
	}

private:
	const StoreContainer	*list_;
};


} // namespace multi
} // namespace

#endif

