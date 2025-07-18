#include "listloader/singlelistloader.h"
#include "listdbadapter.h"

namespace DBAdapterFactory{

	using hm4::disk::DiskList;

	struct SingleList{
		using ListLoader		= hm4::listloader::SingleListLoader;

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
		SingleList(UStringPathData &&path_data, DiskList::VMAllocator &allocator) :
						loader_{
							std::forward<UStringPathData>(path_data),
							&allocator
						},
						adapter_{
							loader_.getList(),
							/* cmd Save   */ loader_,
							/* cmd Reload */ loader_
						}{}

		template<typename UStringPathData>
		SingleList(UStringPathData &&path_data, DiskList::NoVMAllocator) :
						loader_{
							std::forward<UStringPathData>(path_data),
							nullptr
						},
						adapter_{
							loader_.getList(),
							/* cmd Save   */ loader_,
							/* cmd Reload */ loader_
						}{}

		MyDBAdapter &operator()(){
			return adapter_;
		}

	private:
		ListLoader		loader_;
		DBAdapter		adapter_;
	};

}

