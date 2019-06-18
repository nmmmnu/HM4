#include "listloader/directorylistloader.h"
#include "multi/collectionlist.h"

#include "skiplist.h"
#include "idgenerator/idgeneratordate.h"
#include "flusher/diskfileflusher.h"
#include "flushlist.h"

#include "multi/duallist.h"

#include "listdbadapter.h"

#include <vector>

struct MyMutableDBAdapterFactory{
	using ListLoader	= hm4::listloader::DirectoryListLoader;

	using MemList		= hm4::SkipList;
	using IDGenerator	= hm4::idgenerator::IDGeneratorDate;
	using Flusher		= hm4::flusher::DiskFileFlusher<IDGenerator>;
	using MutableFlushList	= hm4::FlushList<MemList, Flusher, ListLoader>;

	using DList		= hm4::multi::DualList<MutableFlushList, ListLoader::List, /* erase tombstones */ true>;

	using CommandObject	= MutableFlushList;
	using DBAdapter		= ListDBAdapter<DList, CommandObject>;

	using MyDBAdapter	= DBAdapter;

	MyMutableDBAdapterFactory(const StringRef &path, size_t const memListSize) :
					loader_(path),
					muflList_(
						memList_,
						Flusher{ IDGenerator{}, path },
						loader_,
						memListSize
					),
					list_(muflList_, loader_.getList()),
					adapter_(list_, /* cmd */ muflList_){}

	auto &operator()(){
		return adapter_;
	}

private:
	ListLoader		loader_;
	MemList			memList_;
	MutableFlushList	muflList_;
	DList			list_;
	DBAdapter		adapter_;
};

