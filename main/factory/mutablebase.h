#ifndef MUTABLE_BASE_H_
#define MUTABLE_BASE_H_


#include "listloader/directorylistloader.h"
#include "multi/collectionlist.h"

#include "idgenerator/idgeneratordate.h"
#include "flusher/diskfilepredicate.h"
#include "flusher/diskfileflush.h"
#include "flushlist.h"

#include "multi/duallist.h"

#include "listdbadapter.h"

namespace DBAdapterFactory{

	template<class MemListType>
	struct MutableBase{
		using ListLoader		= hm4::listloader::DirectoryListLoader;

		using MemList			= MemListType;
		using Predicate			= hm4::flusher::DiskFilePredicate;
		using IDGenerator		= hm4::idgenerator::IDGeneratorDate;
		using Flush			= hm4::flusher::DiskFileFlush<IDGenerator>;
		using MutableFlushList		= hm4::FlushList<MemList, Predicate, Flush, ListLoader>;

		using DList			= hm4::multi::DualList<MutableFlushList, ListLoader::List, /* erase tombstones */ true>;

		using CommandSaveObject		= MutableFlushList;
		using CommandReloadObject	= ListLoader;

		using DBAdapter			= ListDBAdapter<
							DList,
							CommandSaveObject,
							CommandReloadObject
						>;

		using MyDBAdapter		= DBAdapter;

		template<typename UStringPathData>
		MutableBase(UStringPathData &&path_data, size_t const memListSize, MemList &memList) :
						loader_{
							std::forward<UStringPathData>(path_data)
						},
						muFlushList_{
							memList,
							Predicate{ memListSize },
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

