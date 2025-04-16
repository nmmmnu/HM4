#ifndef FLUSH_LIST_BASE_H_
#define FLUSH_LIST_BASE_H_

#include "mybuffer.h"
#include "mmapbuffer.h"

#include "logger.h"

#include <type_traits>
//#include <algorithm>

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
	void save(List &list, Flusher &flusher, MyBuffer::ByteBufferView bufferHash){
		// this is can run in main thread or in save thread,
		// but no guard needed,
		// because thread is join every time before call to this.

		if constexpr(HasPrepareFlush<List>::value){
			list.prepareFlush();
		}

		const auto &clist = list;

		if (bufferHash)
			flusher(clist, bufferHash);
		else
			flusher(clist);
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
	InsertResult flushThenInsert(FlushList &flushList, PFactory &factory, FlushContext &context){
		// this is single thread, no guard needed

		auto bufferPair = context.bufferPair;

		MyBuffer::AdviceNeededGuard guard{ bufferPair };

		Pair *pair = reinterpret_cast<Pair *>(bufferPair.data());

		factory.create(pair);

		logger<Logger::NOTICE>()
				<< "Save referenced data in the pair."
				<< "Pair size:" << pair->bytes();

		// this can block, but usually happens immediately.

		flushList.flush();

		logger<Logger::NOTICE>() << "Clone saved pair.";

		PairFactory::Clone cloneFactory{ pair };

		return flushList.insertF_(cloneFactory);
	}

	template<class FlushList, class InsertList, class Predicate, class PFactory>
	InsertResult insertF(FlushList &flushList, InsertList const &insertList, Predicate &predicate, PFactory &factory, FlushContext &context){
		// this is single thread, no guard needed

		if (!factory.valid())
			return InsertResult::errorInvalid();

		if (predicate(insertList, factory.bytes()))
			return flushThenInsert(flushList, factory, context);

		if (auto const result = flushList.insertF_(factory); result.status != result.ERROR_NO_MEMORY)
			return result;

		// ERROR_NO_MEMORY
		//
		// Unlikely, because we checked with the predicate already, but just in case
		//
		// The list guarantee, no changes on the list are done, if ERROR_NO_MEMORY

		return flushThenInsert(flushList, factory, context);
	}

} // namespace namespace hm4::flushlist_impl_

#endif

