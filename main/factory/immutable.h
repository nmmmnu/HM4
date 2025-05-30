#include "listloader/directorylistloader.h"
#include "multi/collectionlist.h"
#include "listdbadapter.h"

namespace DBAdapterFactory{

	using hm4::disk::DiskList;

	struct Immutable{
		using ListLoader		= hm4::listloader::DirectoryListLoader;

		using CommandObject		= ListLoader;
		using CommandSaveObject		= CommandObject;
		using CommandReloadObject	= CommandObject;


		using DBAdapter			= ListDBAdapter<
							ListLoader::List,
							CommandSaveObject,
							CommandReloadObject
						>;

		using MyDBAdapter		= DBAdapter;

		template<typename UStringPathData>
		Immutable(UStringPathData &&path_data, DiskList::VMAllocator &allocator) :
						loader_{
							std::forward<UStringPathData>(path_data),
							&allocator
						},
						adapter_{
							loader_.getList(),
							/* cmd Save   */ loader_,
							/* cmd Reload */ loader_
						}{}

		auto &operator()(){
			return adapter_;
		}

	private:
		ListLoader		loader_;
		DBAdapter		adapter_;
	};

}

