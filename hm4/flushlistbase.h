#ifndef FLUSH_LIST_BASE_H_
#define FLUSH_LIST_BASE_H_

#include "staticbuffer.h"
#include "mmapbuffer.h"

#include "logger.h"

#include <type_traits>

namespace hm4::flushlist_impl_{



	template <typename, typename = void>
	struct HasPrepareFlush : std::false_type {};

	template <typename List>
	struct HasPrepareFlush<
				List,
				std::void_t<decltype(std::declval<List>().prepareFlush() )>
			> : std::true_type{};



	template <class ListLoader>
	bool notifyLoader(ListLoader *loader){
		if constexpr(std::is_same_v<ListLoader, std::nullptr_t>){
			return true;
		}else{
			if (loader){
				logger<Logger::NOTICE>() << "Reloading data...";
				loader->refresh();
			}else{
				logger<Logger::NOTICE>() << "No need for reloading data...";
			}

			return true;
		}
	}

	template<class List, class Flusher>
	void save(List &list, Flusher &flusher, hm4::disk::FileBuilder::FileBuilderWriteBuffers &buffersWrite, MyBuffer::ByteBufferView bufferHash){
		// this code may runs in main thread or in save thread,
		// but no guard needed,
		// because thread is join-ed every time before call to this.

		if constexpr(HasPrepareFlush<List>::value){
			list.prepareFlush();
		}

		const auto &clist = list;

		if (bufferHash)
			flusher(clist, buffersWrite, bufferHash);
		else
			flusher(clist, buffersWrite);
	}

	template<class List, class ListLoader>
	void clear(List &list, ListLoader *loader){
		list.clear();
		notifyLoader(loader);
	}



	struct FlushContext{
		MyBuffer::ByteBufferView bufferPair;
	};



	template<class FlushList, class PFactory>
	InsertResult flushThenInsert(FlushList &flushList, PFactory &factory, MyBuffer::ByteBufferView bufferPair){
		// this is single thread, no guard needed

		Pair *pair = reinterpret_cast<Pair *>(bufferPair.data());

		factory.create(pair);

		logger<Logger::NOTICE>()
				<< "Save referenced data in the pair."
				<< "Pair size:" << pair->bytes();

		// this may block, but usually happens immediately.

		flushList.flush();

		logger<Logger::NOTICE>() << "Clone saved pair.";

		PairFactory::Clone cloneFactory{ pair };

		return flushList.insertF_(cloneFactory);
	}

	template<class FlushList, class PFactory>
	InsertResult flushThenInsertWithContext(FlushList &flushList, PFactory &factory, FlushContext &context){
		// this is single thread, no guard needed

		constexpr size_t microBufferSize = 4096;

		if (factory.bytes() <= microBufferSize){
			// using micro buffer

			MyBuffer::StaticMemoryResource<microBufferSize> bufferPair;

			return flushThenInsert(flushList, factory, bufferPair);
		}else{
			// using MMAP buffer from the context

			auto bufferPair = context.bufferPair;

			MyBuffer::MMapAdviceGuard guard{ bufferPair };

			return flushThenInsert(flushList, factory, bufferPair);
		}
	}

	template<class FlushList, class InsertList, class Predicate, class PFactory>
	InsertResult insertF(FlushList &flushList, InsertList const &insertList, Predicate &predicate, PFactory &factory, FlushContext &context){
		// this is single thread, no guard needed

		if (!factory.valid())
			return InsertResult::errorInvalid();

		if (predicate(insertList, factory.bytes()))
			return flushThenInsertWithContext(flushList, factory, context);

		if (auto const result = flushList.insertF_(factory); result.status != result.ERROR_NO_MEMORY)
			return result;

		// ERROR_NO_MEMORY
		//
		// Unlikely, because we checked with the predicate already, but just in case
		//
		// The list guarantee, no changes on the list are done, if ERROR_NO_MEMORY

		return flushThenInsertWithContext(flushList, factory, context);
	}

} // namespace namespace hm4::flushlist_impl_

#endif

