#ifndef MUTABLE_BASE_H_
#define MUTABLE_BASE_H_

#include "listloader/directorylistloader.h"
#include "multi/collectionlist.h"

#include "idgenerator.h"
#include "flusher/diskfilepredicate.h"
#include "flusher/diskfileflush.h"
#include "flushlist.h"

#include "multi/duallist.h"

#include "listdbadapter.h"

namespace DBAdapterFactory{

	using hm4::multi::DualListEraseType;

#ifndef USE_CONCURRENCY
	template<
		DualListEraseType ET,
		class MemListType,
		class MyPairBuffer,
		template<                   class, class, class, class, class> class MutableFlushListType
	>
#else
	template<
		DualListEraseType ET,
		class MemListType,
		class MyPairBuffer,
		template<DualListEraseType, class, class, class, class, class> class MutableFlushListType
	>
#endif
	struct MutableBase{
		using ListLoader		= hm4::listloader::DirectoryListLoader;

		using MemList			= MemListType;
		using Predicate			= hm4::flusher::DiskFileAllocatorPredicate;
		using IDGenerator		= idgenerator::IDGeneratorDate;
		using Flush			= hm4::flusher::DiskFileFlush<IDGenerator>;

		#ifndef USE_CONCURRENCY
		using MutableFlushList		= MutableFlushListType<    MemList, MyPairBuffer, Predicate, Flush, ListLoader>;
		#else
		using MutableFlushList		= MutableFlushListType<ET, MemList, MyPairBuffer, Predicate, Flush, ListLoader>;
		#endif

		using DList			= hm4::multi::DualList<
							MutableFlushList,
							ListLoader::List,
							ET
						>;

		using CommandSaveObject		= MutableFlushList;
		using CommandReloadObject	= ListLoader;

		using DBAdapter			= ListDBAdapter<
							DList,
							CommandSaveObject,
							CommandReloadObject
						>;

		using MyDBAdapter		= DBAdapter;

		template<typename UStringPathData, typename... FlushListArgs>
		MutableBase(UStringPathData &&path_data, FlushListArgs&&... args) :
						loader_{
							std::forward<UStringPathData>(path_data)
						},
						muFlushList_{
							std::forward<FlushListArgs>(args)...,
							Predicate{},
							Flush{ IDGenerator{}, path_data },
							loader_
						},
						list_{
							muFlushList_,
							loader_.getList()
						},
						adapter_{
							list_,
							/* cmd Save   */ muFlushList_,
							/* cmd Reload */ loader_
						}{}

		auto &operator()(){
			return adapter_;
		}

	private:
		ListLoader		loader_		;
		MutableFlushList	muFlushList_	;
		DList			list_		;
		DBAdapter		adapter_	;
	};

}

#endif

