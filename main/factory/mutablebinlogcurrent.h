#include "mutablebase.h"
#include "arenaallocator.h"

#include "binlogger/diskfilebinlogger.h"
#include "binloglist.h"

#include "concurrentflushlist.h"

namespace DBAdapterFactory{

	template<DualListEraseType ET, class MyMemList>
	struct MutableBinLogConcurrent{
		using MemList		= MyMemList;
		using BinLogger		= hm4::binlogger::DiskFileBinLogger;
		using BinLogList	= hm4::BinLogList<MemList,BinLogger,/* unlink */ true>;

		using MutableBase_	= MutableBase<ET, BinLogList, hm4::ConcurrentFlushList>;

		using MyDBAdapter	= typename MutableBase_::MyDBAdapter;

		template<typename UStringPathData, typename UStringPathBinLog1, typename UStringPathBinLog2>
		MutableBinLogConcurrent(UStringPathData &&path_data, UStringPathBinLog1 &&path_binlog1, UStringPathBinLog2 &&path_binlog2, BinLogger::SyncOptions const syncOprions, typename MemList::Allocator &allocator1, typename MemList::Allocator &allocator2) :
					memList1_{ allocator1 },
					binLogList1_{
						memList1_,
						BinLogger{
							std::forward<UStringPathBinLog1>(path_binlog1),
							syncOprions,
							hm4::Pair::WriteOptions::ALIGNED
						}
					},
					memList2_{ allocator2 },
					binLogList2_{
						memList2_,
						BinLogger{
							std::forward<UStringPathBinLog2>(path_binlog2),
							syncOprions,
							hm4::Pair::WriteOptions::ALIGNED
						}
					},
					base_{
						std::forward<UStringPathData>(path_data),
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

