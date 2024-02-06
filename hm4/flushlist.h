#ifndef FLUSH_LIST_H_
#define FLUSH_LIST_H_

#include <memory>	// unique_ptr

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

	auto mutable_version() const{
		return version_;
	}

	void flush(){
		save_();

		flushlist_impl_::clear(*list_, loader_);

		++version_;
	}

	// Command pattern
	bool command(){
		flush();

		return true;
	}

	template<class PFactory>
	auto insertF(PFactory &factory){
		return flushlist_impl_::insertF(*this, *list_, predicate_, factory, *pairBuffer);
	}

	template<class PFactory>
	auto insertF_NoFlush(PFactory &factory){
		return list_->insertF(factory);
	}

private:
	void save_() const{
		logger<Logger::NOTICE>() << "Save data..." << "List record(s):" << list_->size() << "List size:" << list_->bytes();

		flushlist_impl_::save(*list_, flusher_);

		logger<Logger::NOTICE>() << "Save done";
	}

private:
	using multi::SingleList<List>::list_;

	Predicate	predicate_;
	Flusher		flusher_;
	ListLoader	*loader_;

	uint64_t	version_ = 0;

	std::unique_ptr<PairBuffer>	pairBuffer = std::make_unique<PairBuffer>();
};


} // namespace


#endif

