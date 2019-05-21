#include "listloader/directorylistloader.h"
#include "multi/collectionlist.h"
#include "listdbadapter.h"

struct MyImmutableDBAdapterFactory{
	using ListLoader	= hm4::listloader::DirectoryListLoader;
	using ImmutableList	= hm4::multi::CollectionListFromContainer<ListLoader::container_type>;

	using CommandObject	= ListLoader;
	using DBAdapter		= ListDBAdapter<const ImmutableList, CommandObject>;

	using MyDBAdapter	= DBAdapter;

	MyImmutableDBAdapterFactory(const StringRef &path, size_t) :
					loader_(path),
					imList_(loader_.container()),
					adapter_(imList_, /* cmd */ loader_){}

	MyDBAdapter &operator()(){
		return adapter_;
	}

private:
	ListLoader		loader_;
	ImmutableList		imList_;
	DBAdapter		adapter_;
};

