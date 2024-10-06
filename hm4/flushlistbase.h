#ifndef FLUSH_LIST_BASE_H_
#define FLUSH_LIST_BASE_H_

#include <type_traits>
#include <algorithm>

#include "mybufferadvice.h"

#include "logger.h"

namespace hm4{



namespace flushlist_impl_{

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
	void save(List &list, Flusher &flusher){
		if constexpr(HasPrepareFlush<List>::value){
			list.prepareFlush();
		}

		const auto &clist = list;

		flusher(std::begin(clist), std::end(clist));
	}

	template<class List, class ListLoader>
	void clear(List &list, ListLoader *loader){
		list.clear();
		notifyLoader(loader);
	}

	template<class FlushList, class PFactory>
	auto insertF_NoFlush(FlushList &flushList, PFactory &factory){
		// redirection need, because flushlist internal list may updated
		return flushList.insertF_NoFlush(factory);
	}

	template<class FlushList, class PFactory, class MyPairBuffer>
	auto flushThenInsert(FlushList &flushList, PFactory &factory, MyPairBuffer &pairBuffer){
		MyBuffer::AdviceNeededGuard guard(pairBuffer);

		Pair *pair = reinterpret_cast<Pair *>(pairBuffer->data());

		factory.create(pair);

		logger<Logger::NOTICE>() << "Save referenced data in the pair."
			//	<< "key:" << pair->getKey()
				<< "Pair size:" << pair->bytes();

		flushList.flush();

		logger<Logger::NOTICE>() << "Clone saved pair.";

		PairFactory::Clone cloneFactory{ pair };

		return insertF_NoFlush(flushList, cloneFactory);
	}

	template<class FlushList, class InsertList, class Predicate, class PFactory, class MyPairBuffer>
	auto insertF(FlushList &flushList, InsertList &insertList, Predicate &predicate, PFactory &factory, MyPairBuffer &pairBuffer){
		if (!factory.valid())
			return InsertResult::errorInvalid();

		if (predicate(insertList, factory.bytes()))
			return flushThenInsert(flushList, factory, pairBuffer);

		auto const result = insertF_NoFlush(flushList, factory);

		switch(result.status){
		case result.ERROR_NO_MEMORY:	return flushThenInsert(flushList, factory, pairBuffer);	// try insert again
		default:			return result;
		}
	}

} // namespace



} // namespace


#endif

