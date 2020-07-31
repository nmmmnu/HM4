#ifndef FLUSH_LIST_H_
#define FLUSH_LIST_H_

#include "decoratorlist.h"

#include "logger.h"
#include "scopedthread.h"

namespace hm4{


template <class List, class Predicate, class Flusher, class ListLoader = std::nullptr_t>
class ConcurrentFlushList : public DecoratorList<List>{
private:
	template <class UPredicate, class UFlusher>
	ConcurrentFlushList(List &list1, List &list2, UPredicate &&predicate, UFlusher &&flusher, ListLoader *loader) :
					DecoratorList<List>(list1),
						list_		(&list1					),
						listRO_		(&list2					),
						predicate_	(std::forward<UPredicate>(predicate)	),
						flusher_	(std::forward<UFlusher>(flusher)	),
						loader_		(loader					){}

public:
	using Allocator = typename DecoratorList<List>::Allocator;

	template <class UPredicate, class UFlusher>
	ConcurrentFlushList(List &list1, List &list2, UPredicate &&predicate, UFlusher &&flusher, ListLoader &loader) :
					ConcurrentFlushList(list1, list2, std::forward<UPredicate>(predicate), std::forward<UFlusher>(flusher), &loader){}

	template <class UPredicate, class UFlusher>
	ConcurrentFlushList(List &list1, List &list2, UPredicate &&predicate, UFlusher &&flusher) :
					ConcurrentFlushList(list1, list2, std::forward<UPredicate>(predicate), std::forward<UFlusher>(flusher), nullptr){}

	~ConcurrentFlushList(){
		save_(*list_);

		// ScopedThread joins now.
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
		log__("Start Flushing data...");

		// we have to switch lists, so need to join()
		thread_.join();

		using std::swap;
		swap(*list_, *listRO_);

		if (!empty(*listRO_)){
			thread_ = ScopedThread{ [this](){ save_(*listRO_, false); } };
		//	save_(*listRO_);
		}

		if (!empty(*list_)){
			list_->clear();
			notifyLoader_();
		}

		return true;
	}

	// Command pattern
	bool command(){
		return flush();
	}

private:
	void save_(List &list, bool const fg = true) const{
		[[maybe_unused]]
		std::string_view const id = fg ? "Foreground" : "Background";

		log__("TH#", id, "Flushing data...", "List record(s): ", list.size(), "List size: ", list.bytes());

		flusher_(std::begin(list), std::end(list));

		log__("TH#", id, "Flushing done");
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
	List		*listRO_;
	Predicate	predicate_;
	Flusher		flusher_;
	ListLoader	*loader_;

	ScopedThread	thread_;
};


} // namespace


#endif

