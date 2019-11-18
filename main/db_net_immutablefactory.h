#include "listloader/directorylistloader.h"
#include "multi/collectionlist.h"
#include "listdbadapter.h"

#include <vector>

struct MyImmutableDBAdapterFactory{
	using ListLoader	= hm4::listloader::DirectoryListLoader;

	using CommandObject	= ListLoader;
	using DBAdapter		= ListDBAdapter<ListLoader::List, CommandObject>;

	using MyDBAdapter	= DBAdapter;

	MyImmutableDBAdapterFactory(std::string path) :
					loader_(std::move(path)),
					adapter_(loader_.getList(), /* cmd */ loader_){}

	auto &operator()(){
		return adapter_;
	}

private:
	ListLoader		loader_;
	DBAdapter		adapter_;
};

