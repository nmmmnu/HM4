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
		using MemList		= hm4::SkipList;
		using BinLogger		= hm4::binlogger::DiskFileBinLogger;
		using BinLogList	= hm4::BinLogList<MemList,BinLogger>;

		using BasicMutable_	= BasicMutable<BinLogList>;

		using MyDBAdapter	= BasicMutable_::DBAdapter;

		template<typename UStringPathData, typename UStringPathBinLog>
		MutableBinLog(UStringPathData &&path_data, UStringPathBinLog &&path_binlog, bool const fsync_binlog, size_t const memListSize, MyAllocator::PMAllocator &allocator) :
					memList_{ allocator },
					binLogList_{
						memList_,
						BinLogger{
							std::forward<UStringPathBinLog>(path_binlog),
							/* fsync   */ fsync_binlog,
							/* aligned */ true
						}
					},
					base_{
						std::forward<UStringPathData>(path_data),
						memListSize,
						binLogList_
					}{}

		auto &operator()(){
			return base_();
		}

	private:
		MemList			memList_	;
		BinLogList		binLogList_	;

		BasicMutable_		base_		;
	};

}

