#include "listloader/singlelistloader.h"
#include "listdbadapter.h"

namespace DBAdapterFactory{

	struct SingleList{
		using ListLoader	= hm4::listloader::SingleListLoader;

		using CommandObject	= ListLoader;
		using DBAdapter		= ListDBAdapter<ListLoader::List, CommandObject>;

		using MyDBAdapter	= DBAdapter;

		template<typename UString>
		SingleList(UString &&path) :
						loader_(std::forward<UString>(path)),
						adapter_(loader_.getList(), /* cmd */ loader_){}

		MyDBAdapter &operator()(){
			return adapter_;
		}

	private:
		ListLoader		loader_;
		DBAdapter		adapter_;
	};

}

