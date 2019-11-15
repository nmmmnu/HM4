#ifndef _Flusher_List_H
#define _Flusher_List_H

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
	template <class UPredicate, class UFlusher>
	FlushList(List &list, UPredicate &&predicate, UFlusher &&flusher, ListLoader &loader) :
					FlushList(list, std::forward<UPredicate>(predicate), std::forward<UFlusher>(flusher), &loader){}

	template <class UPredicate, class UFlusher>
	FlushList(List &list, UPredicate &&predicate, UFlusher &&flusher) :
					FlushList(list, std::forward<UPredicate>(predicate), std::forward<UFlusher>(flusher), nullptr){}



	~FlushList(){
		flush();
	}

	bool insert(	std::string_view const key, std::string_view const val,
			uint32_t const expires = 0, uint32_t const created = 0){

		bool const result = list_->insert(key, val, expires, created );

		if (predicate_(*list_))
			flush();

		return result;
	}

	bool flush(){
		log__("Flushing data...", "List record(s): ", list_->size(), "List size: ", list_->bytes());

		bool const r = flusher_(*list_);

		list_->clear();
		notifyLoader_();

		return r;
	}

	// Command pattern
	int command(bool const completeFlush){
		if (completeFlush)
			return flush();
		else
			return notifyLoader_();
	}

private:
	bool notifyLoader_(std::false_type){
		log__("Reloading data...");
		return loader_ && loader_->refresh();
	}

	static bool notifyLoader_(std::true_type){
		return true;
	}

	bool notifyLoader_(){
		constexpr auto tag = std::is_same<ListLoader, std::nullptr_t>{};
		return notifyLoader_(tag);
	}

private:
	List		*list_;
	Predicate	predicate_;
	Flusher		flusher_;
	ListLoader	*loader_;
};


} // namespace


#endif

