#include "listloader/singlelistloader.h"
#include "listdbadapter.h"

#include "timerlist.h"

namespace DBAdapterFactory{

	using hm4::disk::DiskList;

	struct SingleList{
		using ListLoader		= hm4::listloader::SingleListLoader;

		using CommandObject		= ListLoader;
		using CommandReloadObject	= CommandObject;

		using ImmutableList_timer	= hm4::TimerList<ListLoader::List, 0>;

		using DBAdapter			= ListDBAdapter<
							const ImmutableList_timer,
							CommandReloadObject
						>;

		using MyDBAdapter		= DBAdapter;

		template<typename UStringPathData>
		SingleList(UStringPathData &&path_data, DiskList::VMAllocator &allocator) :
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

		template<typename UStringPathData>
		SingleList(UStringPathData &&path_data, DiskList::NoVMAllocator) :
						loader_{
							std::forward<UStringPathData>(path_data),
							nullptr
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

