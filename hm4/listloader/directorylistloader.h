#ifndef DIRECTORY_LIST_LOADER_H_
#define DIRECTORY_LIST_LOADER_H_

#include "baselistloader.h"

namespace hm4{
namespace listloader{

class DirectoryListLoader{
public:
	using DiskList	= hm4::disk::DiskList;
	using List 	= const impl_::ContainerHelper::CollectionList;

public:
	DirectoryListLoader(std::string path, MMAPFile::Advice const advice = DiskList::DEFAULT_ADVICE, DiskList::OpenMode const mode = DiskList::DEFAULT_MODE) :
				container_(advice, mode),
				path_(std::move(path)){
		refresh_();
	}

	bool refresh(){
		refresh_();

		return true;
	}

	// Command pattern
	int command(int = 0){
		return refresh();
	}

	/* const */ List &getList() const{
		return container_.getList();
	}

public:
	static bool checkIfLoaderNeed(StringRef const &s);

private:
	void refresh_();

private:
	impl_::ContainerHelper	container_;

	std::string		path_;
};


} // namespace listloader
} // namespace

#endif

