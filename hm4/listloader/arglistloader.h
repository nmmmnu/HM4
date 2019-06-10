#ifndef ARG_LIST_LOADER_H_
#define ARG_LIST_LOADER_H_

#include "baselistloader.h"

#include "mynarrow.h"

namespace hm4{
namespace listloader{


class ArgListLoader{
public:
	using DiskList	= hm4::disk::DiskList;
	using List 	= const impl_::ContainerHelper::CollectionList;

public:
	ArgListLoader(int const argc, const char **argv, MMAPFile::Advice const advice = DiskList::DEFAULT_ADVICE, DiskList::OpenMode const mode = DiskList::DEFAULT_MODE) :
				container_(advice, mode),
				argc_(argc),
				argv_(argv){
		refresh();
	}

	bool refresh(){
		container_.clear();
		container_.reserve(narrow<size_t>(argc_));

		for(int i = argc_; i --> 0;)
			container_.push_back(argv_[i]);

		return true;
	}

	// Command pattern
	int command(int = 0){
		return refresh();
	}

	/* const */ List &getList() const{
		return container_.getList();
	}

private:
	impl_::ContainerHelper	container_;

	int			argc_	= 0;
	const char		**argv_;
};


} // namespace listloader
} // namespace

#endif

