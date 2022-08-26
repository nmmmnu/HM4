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
	template<typename UString>
	DirectoryListLoader(UString &&path, MMAPFile::Advice const advice = DiskList::DEFAULT_ADVICE, DiskList::OpenMode const mode = DiskList::DEFAULT_MODE) :
				container_(advice, mode),
				path_(std::forward<UString>(path)){

		if (checkIfLoaderNeed(path_) == false){
			// guard against missing '*'
			stop__(path_);
		}

		refresh();
	}

	void refresh();

	// Command pattern
	bool command(){
		refresh();

		return true;
	}

	List const &getList() const{
		return container_.getList();
	}

public:
	static bool checkIfLoaderNeed(std::string_view const s);

private:
	static void stop__(std::string_view const s);

private:
	impl_::ContainerHelper	container_;

	std::string		path_;

//	std::map<uint64_t, >	inodes_;
};


} // namespace listloader
} // namespace

#endif

