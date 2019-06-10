#include "listloader/singlelistloader.h"
#include "listdbadapter.h"

struct MySingleListDBAdapterFactory{
	using ListLoader	= hm4::listloader::SingleListLoader;

	using CommandObject	= ListLoader;
	using DBAdapter		= ListDBAdapter<const ListLoader::List, CommandObject>;

	using MyDBAdapter	= DBAdapter;

	MySingleListDBAdapterFactory(const StringRef &path, size_t) :
					loader_(path),
					adapter_(loader_.getList(), /* cmd */ loader_){}

	MyDBAdapter &operator()(){
		return adapter_;
	}

private:
	ListLoader		loader_;
	DBAdapter		adapter_;
};

