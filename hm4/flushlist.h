#ifndef FLUSH_LIST_H_
#define FLUSH_LIST_H_

#include "multi/singlelist.h"

#include "flushlistbase.h"

namespace hm4{



template <class List, class Predicate, class Flusher, class ListLoader = std::nullptr_t>
class FlushList : public multi::SingleList<List>{
private:
	using FileBuilderWriteBuffers = hm4::disk::FileBuilder::FileBuilderWriteBuffers;

	template <class UPredicate, class UFlusher>
	FlushList(List &list, FileBuilderWriteBuffers &buffersWrite, MyBuffer::ByteBufferView bufferPair, MyBuffer::ByteBufferView bufferHash, UPredicate &&predicate, UFlusher &&flusher, ListLoader *loader) :
					multi::SingleList<List>(list),
						predicate_	(std::forward<UPredicate>(predicate)	),
						flusher_	(std::forward<UFlusher>(flusher)	),
						loader_		(loader					),
						buffersWrite_	(&buffersWrite				),
						bufferPair_	(bufferPair				),
						bufferHash_	(bufferHash				){}

public:
	using Allocator = typename multi::SingleList<List>::Allocator;

	template <class UPredicate, class UFlusher>
	FlushList(List &list, FileBuilderWriteBuffers &buffersWrite, MyBuffer::ByteBufferView bufferPair, MyBuffer::ByteBufferView bufferHash, UPredicate &&predicate, UFlusher &&flusher, ListLoader &loader) :
					FlushList(list, buffersWrite, bufferPair, bufferHash, std::forward<UPredicate>(predicate), std::forward<UFlusher>(flusher), &loader){}

	template <class UPredicate, class UFlusher>
	FlushList(List &list, FileBuilderWriteBuffers &buffersWrite, MyBuffer::ByteBufferView bufferPair, MyBuffer::ByteBufferView bufferHash, UPredicate &&predicate, UFlusher &&flusher) :
					FlushList(list, buffersWrite, bufferPair, bufferHash, std::forward<UPredicate>(predicate), std::forward<UFlusher>(flusher), nullptr){}

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
		FlushContext context{
			bufferPair_
		};

		return flushlist_impl_::insertF(*this, *list_, predicate_, factory, context);
	}

private:
	using FlushContext = flushlist_impl_::FlushContext;

	template<class FlushList1, class PFactory>
	friend InsertResult flushlist_impl_::flushThenInsert(FlushList1 &flushList, PFactory &factory, MyBuffer::ByteBufferView bufferPair);

	template<class FlushList1, class InsertList, class Predicate1, class PFactory>
	friend InsertResult flushlist_impl_::insertF(FlushList1 &flushList, InsertList const &insertList, Predicate1 &predicate, PFactory &factory, FlushContext &context);

private:
	template<class PFactory>
	auto insertF_(PFactory &factory){
		return list_->insertF(factory);
	}

	void save_() const{
		logger<Logger::NOTICE>() << "Save data..." << "List record(s):" << list_->size() << "List size:" << list_->bytes();

		flushlist_impl_::save(*list_, flusher_, *buffersWrite_, bufferHash_);

		logger<Logger::NOTICE>() << "Save done";
	}

private:
	using multi::SingleList<List>::list_;

	Predicate			predicate_;
	Flusher				flusher_;
	ListLoader			*loader_;

	uint64_t			version_ = 0;

	FileBuilderWriteBuffers		*buffersWrite_;

	MyBuffer::ByteBufferView	bufferPair_;
	MyBuffer::ByteBufferView	bufferHash_;
};


} // namespace


#endif

