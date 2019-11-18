#include "listloader/directorylistloader.h"
#include "multi/collectionlist.h"

#include "skiplist.h"
#include "idgenerator/idgeneratordate.h"
#include "flusher/diskfilepredicate.h"
#include "flusher/diskfileflush.h"
#include "flushlist.h"

#include "multi/duallist.h"

#include "listdbadapter.h"

struct MyMutableDBAdapterFactory{
	using ListLoader	= hm4::listloader::DirectoryListLoader;

	using MemList		= hm4::SkipList;
	using Predicate		= hm4::flusher::DiskFilePredicate;
	using IDGenerator	= hm4::idgenerator::IDGeneratorDate;
	using Flush		= hm4::flusher::DiskFileFlush<IDGenerator>;
	using MutableFlushList	= hm4::FlushList<MemList, Predicate, Flush, ListLoader>;

	using DList		= hm4::multi::DualList<MutableFlushList, ListLoader::List, /* erase tombstones */ true>;

	using CommandObject	= MutableFlushList;
	using DBAdapter		= ListDBAdapter<DList, CommandObject>;

	using MyDBAdapter	= DBAdapter;

	MyMutableDBAdapterFactory(std::string const &path, size_t const memListSize, MyAllocator::PMAllocator &allocator) :
					loader_(path),
					memList_(allocator),
					muflList_{
						memList_,
						Predicate{ memListSize },
						Flush{ IDGenerator{}, path },
						loader_
					},
					list_(muflList_, loader_.getList()),
					adapter_(list_, /* cmd */ muflList_){}

	auto &operator()(){
		return adapter_;
	}

private:
	ListLoader		loader_		;
	MemList			memList_	;
	MutableFlushList	muflList_	;
	DList			list_		;
	DBAdapter		adapter_	;
};

