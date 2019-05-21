#include "listloader/directorylistloader.h"
#include "multi/collectionlist.h"
#include "listdbadapter.h"

#include <vector>

struct MyImmutableDBAdapterFactory{
	using Container		= std::vector<hm4::disk::DiskList>;
	using ListLoader	= hm4::listloader::DirectoryListLoader<Container>;
	using ImmutableList	= hm4::multi::CollectionListFromContainer<Container>;

	using CommandObject	= ListLoader;
	using DBAdapter		= ListDBAdapter<const ImmutableList, CommandObject>;

	using MyDBAdapter	= DBAdapter;

	MyImmutableDBAdapterFactory(const StringRef &path, size_t) :
					loader_(container_, path),
					imList_(container_),
					adapter_(imList_, /* cmd */ loader_){}

	MyDBAdapter &operator()(){
		return adapter_;
	}

private:
	Container		container_;
	ListLoader		loader_;
	ImmutableList		imList_;
	DBAdapter		adapter_;
};

