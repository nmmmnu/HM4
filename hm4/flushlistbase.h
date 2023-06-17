#ifndef FLUSH_LIST_BASE_H_
#define FLUSH_LIST_BASE_H_

#include <type_traits>
#include <algorithm>

#include "logger.h"

namespace hm4{



namespace flushlist_impl_{

	template <typename, typename = void>
	class HasPrepareFlush : public std::false_type {};

	template <typename List>
	class HasPrepareFlush<
				List,
				std::void_t<decltype(std::declval<List>().prepareFlush() )>
			> : public std::true_type{};



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
}



} // namespace


#endif

