#ifndef COLLECTION_LIST_H_
#define COLLECTION_LIST_H_

#include "ilist.h"

#include "collectioniterator.h"

#include <numeric>	// std::accumulate

namespace hm4::multi{

namespace collectionlist_impl_{
	template<class List, class = void>
	struct conf_always_non_empty : std::false_type{};

	template<class List>
	struct conf_always_non_empty<List, std::void_t<decltype(List::conf_always_non_empty)> >: std::bool_constant<List::conf_always_non_empty>{};

	// -------

	template<class List, class = void>
	struct conf_no_crontab : std::false_type{};

	template<class List>
	struct conf_no_crontab<List, std::void_t<decltype(List::conf_no_crontab)> >: std::bool_constant<List::conf_no_crontab>{};
}



template <class StoreContainer>
class CollectionList{
public:
	using List			= typename StoreContainer::value_type;

	using size_type			= typename List::size_type;
	using difference_type		= typename List::difference_type;

	using iterator			= CollectionIterator<typename List::iterator>;

	constexpr static bool conf_estimated_size	= true;

public:
	CollectionList(const StoreContainer &list) : list_	(&list){}

	iterator begin() const{
		return { std::begin(*list_), std::end(*list_), std::true_type{} };
	}

	iterator end() const{
		return { std::begin(*list_), std::end(*list_), std::false_type{} };
	}

	iterator find(std::string_view const key) const{
		return { std::begin(*list_), std::end(*list_), key };
	}

	const Pair *getPair(std::string_view const key) const{
		auto first = std::begin(*list_);
		auto last  = std::end(*list_);

		// this is simillar to std::min_element...

		if (first == last){
			// not found. done.
			return nullptr;
		}

		const Pair *smallest = nullptr;

		for(; first != last; ++first){
			if (!smallest){
				smallest = first->getPair(key);
				continue;
			}

			const auto *p = first->getPair(key);

			if (!p)
				continue;

			smallest = smallest->cmpTime(*p) > 0 ? smallest : p;
		}

		return smallest;
	}

public:
	size_type size() const{
		using T = size_type;

		auto sum = [](T const result, List const &list){
			return result + list.size();
		};

		return std::accumulate(std::begin(*list_), std::end(*list_), T{ 0 }, sum);
	}

	bool empty() const{
		using tag = collectionlist_impl_::conf_always_non_empty<List>;

		return empty_(tag{});
	}

	size_t bytes() const{
		using T = size_t;

		auto sum = [](T const result, List const &list){
			return result + list.bytes();
		};

		return std::accumulate(std::begin(*list_), std::end(*list_), T{ 0 }, sum);
	}

	constexpr void crontab() const{
		using tag = collectionlist_impl_::conf_no_crontab<List>;

		return crontab_(tag{});
	}

private:
	auto empty_(std::true_type) const{
		return list_->size() == 0;
	}

	auto empty_(std::false_type) const{
		auto f = [](List const &list){
			return !list.empty();
		};

		return std::find_if(std::begin(*list_), std::end(*list_), f) == std::end(*list_);
	}

private:
	constexpr static void crontab_(std::true_type){
	}

	void crontab_(std::false_type) const{
		for(auto const &list : *list_)
			list.crontab();
	}

private:
	const StoreContainer	*list_;
};



} // namespace multi

#endif

