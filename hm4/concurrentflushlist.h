#ifndef CONCURREN_FLUSH_LIST_H_
#define CONCURREN_FLUSH_LIST_H_

#include "multi/duallist.h"

#include "scopedthread.h"

#include "flushlistbase.h"

namespace hm4{



template <hm4::multi::DualListEraseType ET, class List>
using ConcurrentFlushListBase = multi::DualList<List, List, ET>;



template <hm4::multi::DualListEraseType ET, class List, class Predicate, class Flusher, class ListLoader = std::nullptr_t>
class ConcurrentFlushList : public ConcurrentFlushListBase<ET, List>{
private:
	template <class UPredicate, class UFlusher>
	ConcurrentFlushList(List &list1, List &list2, MyBuffer::ByteBufferView bufferPair, MyBuffer::ByteBufferView bufferHash, UPredicate &&predicate, UFlusher &&flusher, ListLoader *loader) :
					ConcurrentFlushListBase<ET, List>(list1, list2),
						predicate_	(std::forward<UPredicate>(predicate)	),
						flusher_	(std::forward<UFlusher>(flusher)	),
						loader_		(loader					),
						bufferPair_	(bufferPair				),
						bufferHash_	(bufferHash				){}

public:
	using Base = ConcurrentFlushListBase<ET, List>;
	using Allocator = typename Base::Allocator;

	template <class UPredicate, class UFlusher>
	ConcurrentFlushList(List &list1, List &list2, MyBuffer::ByteBufferView bufferPair, MyBuffer::ByteBufferView bufferHash, UPredicate &&predicate, UFlusher &&flusher, ListLoader &loader) :
					ConcurrentFlushList(list1, list2, bufferPair, bufferHash, std::forward<UPredicate>(predicate), std::forward<UFlusher>(flusher), &loader){}

	template <class UPredicate, class UFlusher>
	ConcurrentFlushList(List &list1, List &list2, MyBuffer::ByteBufferView bufferPair, MyBuffer::ByteBufferView bufferHash, UPredicate &&predicate, UFlusher &&flusher) :
					ConcurrentFlushList(list1, list2, bufferPair, bufferHash, std::forward<UPredicate>(predicate), std::forward<UFlusher>(flusher), nullptr){}

	~ConcurrentFlushList(){
		// block, if thread works
		thread_.join();

		save_(*list1_);
	}

	auto mutable_version() const{
		return version_;
	}


	// Command pattern
	bool command(){
		flush();

		return true;
	}

	template<class PFactory>
	auto insertF(PFactory &factory){
		using namespace flushlist_impl_;

		FlushContext context{
			bufferPair_
		};

		return flushlist_impl_::insertF(*this, *list1_, predicate_, factory, context);
	}

	void flush(){
		if (empty(*list1_)){
			logger<Logger::NOTICE>() << "No data for flushing.";
			return;
		}

		logger<Logger::NOTICE>() << "Start Flushing data...";

		// block, if thread works
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

		++version_;
	}

private:
	using FlushContext = flushlist_impl_::FlushContext;

	template<class FlushList1, class PFactory>
	friend InsertResult flushlist_impl_::flushThenInsert(FlushList1 &flushList, PFactory &factory, FlushContext &context);

	template<class FlushList1, class InsertList, class Predicate1, class PFactory>
	friend InsertResult flushlist_impl_::insertF(FlushList1 &flushList, InsertList const &insertList, Predicate1 &predicate, PFactory &factory, FlushContext &context);

private:
	template<class PFactory>
	auto insertF_(PFactory &factory){
		return list1_->insertF(factory);
	}

	void save_(List &list, bool const fg = true) const{
		[[maybe_unused]]
		std::string_view const id = fg ? "Foreground" : "Background";

		logger<Logger::NOTICE>() << "TH#" << id << "Flushing data..." << "List record(s): " << list.size() << "List size: " << list.bytes();

		flushlist_impl_::save(list, flusher_, bufferHash_);

		logger<Logger::NOTICE>() << "TH#" << id << "Flushing done";
	}

private:
	using Base::list1_;
	using Base::list2_;

	Predicate			predicate_;
	Flusher				flusher_;
	ListLoader			*loader_;

	uint64_t			version_ = 0;

	MyBuffer::ByteBufferView	bufferPair_;
	MyBuffer::ByteBufferView	bufferHash_;

	ScopedThread			thread_;
};



} // namespace



#endif

