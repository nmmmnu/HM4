#include "listloader/directorylistloader.h"
#include "multi/collectionlist.h"
#include "listdbadapter.h"

#include "timerlist.h"

namespace DBAdapterFactory{

	using hm4::disk::DiskList;

	struct Immutable{
		using ListLoader		= hm4::listloader::DirectoryListLoader;

		using CommandObject		= ListLoader;
		using CommandReloadObject	= CommandObject;

		using ImmutableList_timer	= hm4::TimerList<ListLoader::List, 0>;

		using DBAdapter			= ListDBAdapter<
							const ImmutableList_timer,
							CommandReloadObject
						>;

		using MyDBAdapter		= DBAdapter;

		template<typename UStringPathData>
		Immutable(UStringPathData &&path_data, DiskList::VMAllocator &allocator) :
						loader_{
							std::forward<UStringPathData>(path_data),
							&allocator
						},
						immutableList_timer_{
							loader_.getList()
						},
						adapter_{
							immutableList_timer_,
							/* cmd Reload */ loader_
						}{}

		auto &operator()(){
			return adapter_;
		}

	private:
		ListLoader		loader_			;
		ImmutableList_timer	immutableList_timer_	;
		DBAdapter		adapter_		;
	};

}

