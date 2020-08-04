#ifndef CONCURREN_FLUSH_LIST_H_
#define CONCURREN_FLUSH_LIST_H_

#include "multi/duallist.h"

#include "logger.h"
#include "scopedthread.h"

namespace hm4{



template <class List>
using ConcurrentFlushListBase = multi::DualListRW<List, List, hm4::multi::DualListEraseType::TOMBSTONE>;



template <class List, class Predicate, class Flusher, class ListLoader = std::nullptr_t>
class ConcurrentFlushList : public ConcurrentFlushListBase<List>{
private:
	template <class UPredicate, class UFlusher>
	ConcurrentFlushList(List &list1, List &list2, UPredicate &&predicate, UFlusher &&flusher, ListLoader *loader) :
					ConcurrentFlushListBase<List>(list1, list2),
						predicate_	(std::forward<UPredicate>(predicate)	),
						flusher_	(std::forward<UFlusher>(flusher)	),
						loader_		(loader					){}

public:
	using Allocator = typename ConcurrentFlushListBase<List>::Allocator;

	template <class UPredicate, class UFlusher>
	ConcurrentFlushList(List &list1, List &list2, UPredicate &&predicate, UFlusher &&flusher, ListLoader &loader) :
					ConcurrentFlushList(list1, list2, std::forward<UPredicate>(predicate), std::forward<UFlusher>(flusher), &loader){}

	template <class UPredicate, class UFlusher>
	ConcurrentFlushList(List &list1, List &list2, UPredicate &&predicate, UFlusher &&flusher) :
					ConcurrentFlushList(list1, list2, std::forward<UPredicate>(predicate), std::forward<UFlusher>(flusher), nullptr){}

	~ConcurrentFlushList(){
		save_(*list1_);

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
		auto result = this->fixDualIterator_(
			list1_->insert(std::move(newdata))
		);

		if (predicate_(*list1_))
			flush();

		return result;
	}

	bool flush(){
		log__("Start Flushing data...");

		// we have to switch lists, so need to join()
		thread_.join();

		using std::swap;
		swap(list1_, list2_);

		if (!empty(*list2_)){
			thread_ = ScopedThread{ [this](){ save_(*list2_, false); } };
		//	save_(*list2_);
		}

		if (!empty(*list1_)){
			list1_->clear();
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
	using		ConcurrentFlushListBase<List>::list1_;
	using		ConcurrentFlushListBase<List>::list2_;

	Predicate	predicate_;
	Flusher		flusher_;
	ListLoader	*loader_;

	ScopedThread	thread_;
};



} // namespace



#endif

