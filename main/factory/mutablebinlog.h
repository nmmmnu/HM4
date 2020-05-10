#include "listloader/directorylistloader.h"
#include "multi/collectionlist.h"

#include "skiplist.h"

#include "binlogger/diskfilebinlogger.h"
#include "binloglist.h"

#include "idgenerator/idgeneratordate.h"
#include "flusher/diskfilepredicate.h"
#include "flusher/diskfileflush.h"
#include "flushlist.h"

#include "multi/duallist.h"

#include "listdbadapter.h"

namespace DBAdapterFactory{

	struct MutableBinLog{
		using ListLoader	= hm4::listloader::DirectoryListLoader;

		using MemList		= hm4::SkipList;

		using BinLogger		= hm4::binlogger::DiskFileBinLogger;
		using BinLogList	= hm4::BinLogList<MemList,BinLogger>;

		using Predicate		= hm4::flusher::DiskFilePredicate;
		using IDGenerator	= hm4::idgenerator::IDGeneratorDate;
		using Flush		= hm4::flusher::DiskFileFlush<IDGenerator>;
		using MutableFlushList	= hm4::FlushList<BinLogList, Predicate, Flush, ListLoader>;

		using DList		= hm4::multi::DualList<MutableFlushList, ListLoader::List, /* erase tombstones */ true>;

		using CommandObject	= MutableFlushList;
		using DBAdapter		= ListDBAdapter<DList, CommandObject>;

		using MyDBAdapter	= DBAdapter;

		template<typename UStringData, typename UStringBinLog>
		MutableBinLog(UStringData &&path_data, UStringBinLog &&path_binlog, size_t const memListSize, MyAllocator::PMAllocator &allocator) :
						loader_(std::forward<UStringData>(path_data)),
						memList_(allocator),
						binLogList_{
							memList_,
							BinLogger{
								std::forward<UStringBinLog>(path_binlog),
								/* aligned */ true
							}
						},
						muFlushList_{
							binLogList_,
							Predicate{ memListSize },
							Flush{ IDGenerator{}, path_data },
							loader_
						},
						list_{
							muFlushList_,
							loader_.getList()
						},
						adapter_{ list_, /* cmd */ muFlushList_ }{}

		auto &operator()(){
			return adapter_;
		}

	private:
		ListLoader		loader_		;
		MemList			memList_	;
		BinLogList		binLogList_	;
		MutableFlushList	muFlushList_	;
		DList			list_		;
		DBAdapter		adapter_	;
	};

}

