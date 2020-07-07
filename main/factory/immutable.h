#include "listloader/directorylistloader.h"
#include "multi/collectionlist.h"
#include "listdbadapter.h"

#include <vector>

namespace DBAdapterFactory{

	struct Immutable{
		using ListLoader	= hm4::listloader::DirectoryListLoader;

		using CommandObject	= ListLoader;

		using DBAdapter		= ListDBAdapter<
						ListLoader::List,
						CommandObject,
						CommandObject
					>;

		using MyDBAdapter	= DBAdapter;

		template<typename UString>
		Immutable(UString &&path) :
						loader_(std::forward<UString>(path)),
						adapter_(
							loader_.getList(),
							/* cmd Save   */ loader_,
							/* cmd Reload */ loader_
						){}

		auto &operator()(){
			return adapter_;
		}

	private:
		ListLoader		loader_;
		DBAdapter		adapter_;
	};

}

