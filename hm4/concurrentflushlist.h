#ifndef CONCURREN_FLUSH_LIST_H_
#define CONCURREN_FLUSH_LIST_H_

#include "multi/duallist.h"

#include "scopedthread.h"

#include "flushlistbase.h"

namespace hm4{



template <class List>
using ConcurrentFlushListBase = multi::DualList<List, List, hm4::multi::DualListEraseType::TOMBSTONE>;



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
		// future improvement
		thread_.join();

		save_(*list1_);
	}

	void flush(){
		if (empty(*list1_)){
			logger<Logger::NOTICE>() << "No data for flushing.";
			return;
		}

		logger<Logger::NOTICE>() << "Start Flushing data...";

		// we have to switch lists, so need to join()
		thread_.join();

		using std::swap;
		swap(list1_, list2_);

		// we already know the list is not empty.
		thread_ = ScopedThread{ [this](){
			 save_(*list2_, false);
		} };

		if (!empty(*list1_)){
			// this also notifiying the loader...
			flushlist_impl_::clear(*list1_, loader_);
		}
	}

	// Command pattern
	bool command(){
		flush();

		return true;
	}

	template<class PFactory>
	auto insertF(PFactory &factory){
		return flushlist_impl_::insertF(*this, *list1_, predicate_, factory);
	}

private:
	void save_(List &list, bool const fg = true) const{
		[[maybe_unused]]
		std::string_view const id = fg ? "Foreground" : "Background";

		logger<Logger::NOTICE>() << "TH#" << id << "Flushing data..." << "List record(s): " << list.size() << "List size: " << list.bytes();

		flushlist_impl_::save(list, flusher_);

		logger<Logger::NOTICE>() << "TH#" << id << "Flushing done";
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

