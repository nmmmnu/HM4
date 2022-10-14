#ifndef FLUSH_LIST_H_
#define FLUSH_LIST_H_

#include "multi/singlelist.h"

#include "flushlistbase.h"

namespace hm4{



template <class List, class Predicate, class Flusher, class ListLoader = std::nullptr_t>
class FlushList : public multi::SingleList<List>{
private:
	template <class UPredicate, class UFlusher>
	FlushList(List &list, UPredicate &&predicate, UFlusher &&flusher, ListLoader *loader) :
					multi::SingleList<List>(list),
						predicate_	(std::forward<UPredicate>(predicate)	),
						flusher_	(std::forward<UFlusher>(flusher)	),
						loader_		(loader					){}

public:
	using Allocator = typename multi::SingleList<List>::Allocator;

	template <class UPredicate, class UFlusher>
	FlushList(List &list, UPredicate &&predicate, UFlusher &&flusher, ListLoader &loader) :
					FlushList(list, std::forward<UPredicate>(predicate), std::forward<UFlusher>(flusher), &loader){}

	template <class UPredicate, class UFlusher>
	FlushList(List &list, UPredicate &&predicate, UFlusher &&flusher) :
					FlushList(list, std::forward<UPredicate>(predicate), std::forward<UFlusher>(flusher), nullptr){}

	~FlushList(){
		save_();
	}

	void flush(){
		save_();

		flushlist_impl_::clear(*list_, loader_);

	//	return true;
	}

	// Command pattern
	bool command(){
		return flush();
	}

	template<class PFactory>
	auto insertLazyPair_(PFactory &&factory){
		auto it = list_->insertLazyPair_(std::move(factory));

		if (predicate_(*list_))
			flush();

		return it;
	}

private:
	void save_() const{
		log__<LogLevel::WARNING>("Save data...", "List record(s): ", list_->size(), "List size: ", list_->bytes());

		flushlist_impl_::save(*list_, flusher_);

		log__<LogLevel::WARNING>("Save done");
	}

private:
	using multi::SingleList<List>::list_;

	Predicate	predicate_;
	Flusher		flusher_;
	ListLoader	*loader_;
};


} // namespace


#endif

