#include "mutablebase.h"
#include "skiplist.h"

#include "binlogger/diskfilebinlogger.h"
#include "binloglist.h"

namespace DBAdapterFactory{

	struct MutableBinLogConcurrent{
		using MemList		= hm4::SkipList;
		using BinLogger		= hm4::binlogger::DiskFileBinLogger;
		using BinLogList	= hm4::BinLogList<MemList,BinLogger>;

		using MutableBase_	= MutableBase<BinLogList, hm4::ConcurrentFlushList>;

		using MyDBAdapter	= MutableBase_::MyDBAdapter;

		template<typename UStringPathData, typename UStringPathBinLog1, typename UStringPathBinLog2>
		MutableBinLogConcurrent(UStringPathData &&path_data, UStringPathBinLog1 &&path_binlog1, UStringPathBinLog2 &&path_binlog2, bool const fsync_binlog, size_t const memListSize, MyAllocator::PMAllocator &allocator1, MyAllocator::PMAllocator &allocator2) :
					memList1_{ allocator1 },
					binLogList1_{
						memList1_,
						BinLogger{
							std::forward<UStringPathBinLog1>(path_binlog1),
							/* fsync   */ fsync_binlog,
							/* aligned */ true
						}
					},
					memList2_{ allocator2 },
					binLogList2_{
						memList2_,
						BinLogger{
							std::forward<UStringPathBinLog2>(path_binlog2),
							/* fsync   */ fsync_binlog,
							/* aligned */ true
						}
					},
					base_{
						std::forward<UStringPathData>(path_data),
						memListSize,
						binLogList1_,
						binLogList2_
					}{}

		auto &operator()(){
			return base_();
		}

	private:
		MemList			memList1_	;
		BinLogList		binLogList1_	;

		MemList			memList2_	;
		BinLogList		binLogList2_	;

		MutableBase_		base_		;
	};

}

