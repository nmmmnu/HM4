#ifndef CONCURREN_FLUSH_LIST_H_
#define CONCURREN_FLUSH_LIST_H_

#include "multi/duallist.h"

#include "logger.h"
#include "scopedthread.h"

#include "flushlistbase.h"

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

		return insert_(key, val, expires, created);
	}

	auto insert(Pair const &src){
		return insert_(src);
	}

	void flush(){
		log__("Start Flushing data...");

		// we have to switch lists, so need to join()
		thread_.join();

		using std::swap;
		swap(list1_, list2_);

		if (!empty(*list2_)){
			thread_ = ScopedThread{ [this](){ save_(*list2_, false); } };
		//	save_(*list2_);
		}

		if (!empty(*list1_))
			flushlist_impl_::clear(*list1_, loader_);
		else
			log__("No data for flushing.");
	}

	// Command pattern
	bool command(){
		flush();

		return true;
	}

private:
	template<typename ...Ts>
	auto insert_(Ts&&... ts){
		auto it = this->fixDualIterator_(
			list1_->insert(std::forward<Ts>(ts)...)
		);

		if (predicate_(*list1_))
			flush();

		return it;
	}

	void save_(List &list, bool const fg = true) const{
		[[maybe_unused]]
		std::string_view const id = fg ? "Foreground" : "Background";

		log__("TH#", id, "Flushing data...", "List record(s): ", list.size(), "List size: ", list.bytes());

		flushlist_impl_::flush(list, flusher_);

		log__("TH#", id, "Flushing done");
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

