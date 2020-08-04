#include "mutablebase.h"
#include "skiplist.h"

#include "binlogger/diskfilebinlogger.h"
#include "binloglist.h"

namespace DBAdapterFactory{

	struct MutableBinLog{
		using MemList		= hm4::SkipList;
		using BinLogger		= hm4::binlogger::DiskFileBinLogger;
		using BinLogList	= hm4::BinLogList<MemList,BinLogger>;

		using MutableBase_	= MutableBase<BinLogList, hm4::FlushList>;

		using MyDBAdapter	= MutableBase_::MyDBAdapter;

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

		MutableBase_		base_		;
	};

}

