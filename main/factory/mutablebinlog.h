#include "mutablebase.h"
#include "arenaallocator.h"

#include "binlogger/diskfilebinlogger.h"
#include "binloglist.h"

namespace DBAdapterFactory{

	template<DualListEraseType ET, class MyMemList>
	struct MutableBinLog{
		using MemList		= MyMemList;
		using BinLogger		= hm4::binlogger::DiskFileBinLogger;
		using BinLogList	= hm4::BinLogList<MemList,BinLogger,/* unlink */ true>;

		using MutableBase_	= MutableBase<ET, BinLogList, hm4::FlushList>;

		using MyDBAdapter	= typename MutableBase_::MyDBAdapter;

		template<typename UStringPathData, typename UStringPathBinLog>
		MutableBinLog(UStringPathData &&path_data, UStringPathBinLog &&path_binlog, BinLogger::SyncOptions const syncOprions, typename MemList::Allocator &allocator, hm4::PairBuffer &pairBuffer) :
					memList_{ allocator },
					binLogList_{
						memList_,
						BinLogger{
							std::forward<UStringPathBinLog>(path_binlog),
							syncOprions,
							hm4::Pair::WriteOptions::ALIGNED
						}
					},
					base_{
						std::forward<UStringPathData>(path_data),
						binLogList_,
						pairBuffer
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

