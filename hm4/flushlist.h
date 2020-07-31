#ifndef FLUSH_LIST_H_
#define FLUSH_LIST_H_

#include "decoratorlist.h"

#include "logger.h"

#include <algorithm>

namespace hm4{


template <class List, class Predicate, class Flusher, class ListLoader = std::nullptr_t>
class FlushList : public DecoratorList<List>{
private:
	template <class UPredicate, class UFlusher>
	FlushList(List &list, UPredicate &&predicate, UFlusher &&flusher, ListLoader *loader) :
					DecoratorList<List>(list),
						list_		(&list					),
						predicate_	(std::forward<UPredicate>(predicate)	),
						flusher_	(std::forward<UFlusher>(flusher)	),
						loader_		(loader					){}

public:
	using Allocator = typename DecoratorList<List>::Allocator;

	template <class UPredicate, class UFlusher>
	FlushList(List &list, UPredicate &&predicate, UFlusher &&flusher, ListLoader &loader) :
					FlushList(list, std::forward<UPredicate>(predicate), std::forward<UFlusher>(flusher), &loader){}

	template <class UPredicate, class UFlusher>
	FlushList(List &list, UPredicate &&predicate, UFlusher &&flusher) :
					FlushList(list, std::forward<UPredicate>(predicate), std::forward<UFlusher>(flusher), nullptr){}

	~FlushList(){
		save_();
	}

	auto insert(	std::string_view const key, std::string_view const val,
			uint32_t const expires = 0, uint32_t const created = 0
			){

		return hm4::insert(*this, key, val, expires, created);
	}

	auto insert(Pair const &src){
		return hm4::insert(*this, src);
	}

	auto insert(typename Pair::smart_ptr::type<Allocator> &&newdata){
		auto result = list_->insert(std::move(newdata));

		if (predicate_(*list_))
			flush();

		return result;
	}

	bool flush(){
		save_();

		list_->clear();
		notifyLoader_();

		return true;
	}

	// Command pattern
	bool command(){
		return flush();
	}

private:
	void save_() const{
		log__("Flushing data...", "List record(s): ", list_->size(), "List size: ", list_->bytes());

		flusher_(std::begin(*list_), std::end(*list_));

		log__("Flushing done");
	}

	bool notifyLoader_(){
		if constexpr(std::is_same_v<ListLoader, std::nullptr_t>){
			return true;
		}else{
			log__("Reloading data...");
			return loader_ && loader_->refresh();
		}
	}

private:
	List		*list_;
	Predicate	predicate_;
	Flusher		flusher_;
	ListLoader	*loader_;
};


} // namespace


#endif

