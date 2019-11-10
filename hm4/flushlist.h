#ifndef _Flusher_List_H
#define _Flusher_List_H

#include "decoratorlist.h"

#include "logger.h"

#include <algorithm>

namespace hm4{


template <class List, class Flusher, class ListLoader = std::nullptr_t>
class FlushList : public DecoratorList<List>{
private:
	constexpr static size_t MAX_SIZE = 128 * 1024 * 1024 * 1ULL;

	template <class UFlusher>
	FlushList(List &list, UFlusher &&flusher, ListLoader *loader, size_t const maxSize) :
					DecoratorList<List>(list),
						list_		(&list					),
						flusher_	(std::forward<UFlusher>(flusher)	),
						loader_		(loader					),
						maxSize_	(std::min(maxSize, MAX_SIZE)		){}

public:
	template <class UFlusher>
	FlushList(List &list, UFlusher &&flusher, ListLoader &loader, size_t const maxSize = MAX_SIZE) :
					FlushList(list, std::forward<UFlusher>(flusher), &loader, maxSize){}

	template <class UFlusher>
	FlushList(List &list, UFlusher &&flusher, size_t const maxSize = MAX_SIZE) :
					FlushList(list, std::forward<UFlusher>(flusher), nullptr, maxSize){}

	~FlushList(){
		flush_();
	}

	bool insert(	std::string_view const key, std::string_view const val,
			uint32_t const expires = 0, uint32_t const created = 0){

		bool const result = list_->insert(key, val, expires, created );

		if (list_->bytes() > maxSize_){
			flush();
		}

		return result;
	}

public:
	bool flush(){
		bool const r = flush_();
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

	bool flush_(){
		log__("Flushing data...", "List size: ", list_->bytes(), "Max permited size: ", maxSize_);
		return flusher_ << *list_;
	}

private:
	List		*list_;
	Flusher		flusher_;
	ListLoader	*loader_;
	size_t		maxSize_;
};


} // namespace


#endif

