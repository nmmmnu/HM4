#include "listloader/directorylistloader.h"
#include "multi/collectionlist.h"

#include "skiplist.h"
#include "idgenerator/idgeneratordate.h"
#include "flusher/diskfileflusher.h"
#include "flushlist.h"

#include "multi/duallist.h"

#include "listdbadapter.h"

struct MyMutableDBAdapterFactory{
	using ListLoader	= hm4::listloader::DirectoryListLoader;
	using ImmutableList	= hm4::multi::CollectionListFromContainer<ListLoader::container_type>;

	using MemList		= hm4::SkipList;
	using IDGenerator	= hm4::idgenerator::IDGeneratorDate;
	using Flusher		= hm4::flusher::DiskFileFlusher<IDGenerator>;
	using MutableFlushList	= hm4::FlushList<MemList,Flusher,ListLoader>;

	using DList		= hm4::multi::DualList<MutableFlushList, ImmutableList, /* erase tombstones */ true>;

	using CommandObject	= MutableFlushList;
	using DBAdapter		= ListDBAdapter<DList, CommandObject>;

	using MyDBAdapter	= DBAdapter;

	MyMutableDBAdapterFactory(const StringRef &path, size_t const memListSize) :
					loader_(path),
					imList_(loader_.container()),
					muflList_(
						memList_,
						Flusher{ IDGenerator{}, path },
						loader_,
						memListSize
					),
					list_(muflList_, imList_),
					adapter_(list_, /* cmd */ muflList_){}

	MyDBAdapter &operator()(){
		return adapter_;
	}

private:
	ListLoader		loader_;
	ImmutableList		imList_;
	MemList			memList_;
	MutableFlushList	muflList_;
	DList			list_;
	DBAdapter		adapter_;
};

