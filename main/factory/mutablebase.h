#ifndef MUTABLE_BASE_H_
#define MUTABLE_BASE_H_

#include "listloader/directorylistloader.h"
#include "multi/collectionlist.h"

#include "idgenerator.h"
#include "flusher/diskfilepredicate.h"
#include "flusher/diskfileflush.h"
#include "flushlist.h"

#include "timerlist.h"

#include "multi/duallist.h"

#include "listdbadapter.h"

namespace DBAdapterFactory{

	using hm4::disk::DiskList;

	using hm4::multi::DualListEraseType;

#ifndef USE_CONCURRENCY
	template<
		DualListEraseType ET,
		class MemListType,
		template<                   class, class, class, class> class MutableFlushListType
	>
#else
	template<
		DualListEraseType ET,
		class MemListType,
		template<DualListEraseType, class, class, class, class> class MutableFlushListType
	>
#endif
	struct MutableBase{
		using ListLoader		= hm4::listloader::DirectoryListLoader;

		using MemList			= MemListType;
		using Predicate			= hm4::flusher::DiskFileAllocatorPredicate;
		using IDGenerator		= idgenerator::IDGeneratorDate;
		using Flush			= hm4::flusher::DiskFileFlush<IDGenerator>;

		#ifndef USE_CONCURRENCY
		using MutableFlushList		= MutableFlushListType<    MemList, Predicate, Flush, ListLoader>;
		#else
		using MutableFlushList		= MutableFlushListType<ET, MemList, Predicate, Flush, ListLoader>;
		#endif

		using MutableFlushList_timer	= hm4::TimerList<MutableFlushList, 0>;

		using ImmutableList_timer	= hm4::TimerList<ListLoader::List, 1>;

		using DList			= hm4::multi::DualList<
							MutableFlushList_timer,
							ImmutableList_timer,
							ET
						>;

		using DList_timer		= hm4::TimerList<DList, 2>;

		using CommandReloadObject	= ListLoader;

		using DBAdapter			= ListDBAdapter<
							DList_timer,
							CommandReloadObject
						>;

		using MyDBAdapter		= DBAdapter;

		template<typename UStringPathData, typename... FlushListArgs>
		MutableBase(uint8_t serverID, UStringPathData &&path_data, DiskList::VMAllocator &slabAllocator, FlushListArgs&&... args) :
						loader_{
							std::forward<UStringPathData>(path_data),
							&slabAllocator
						},
						mutableFlushList_{
							std::forward<FlushListArgs>(args)...,
							Predicate{},
							Flush{ IDGenerator{ serverID }, path_data },
							loader_
						},
						mutableFlushList_timer_{
							mutableFlushList_
						},
						immutableList_timer_{
							loader_.getList()
						},
						list_{
							mutableFlushList_timer_,
							immutableList_timer_
						},
						list_timer_{
							list_
						},
						adapter_{
							list_timer_,
							/* cmd Reload */ loader_
						}{}

		auto &operator()(){
			return adapter_;
		}

	private:
		ListLoader		loader_			;
		MutableFlushList	mutableFlushList_	;
		MutableFlushList_timer	mutableFlushList_timer_	;
		ImmutableList_timer	immutableList_timer_	;
		DList			list_			;
		DList_timer		list_timer_		;
		DBAdapter		adapter_		;
	};

}

#endif

