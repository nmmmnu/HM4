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

		// guard against missing '*'
		checkAndRefresh_();
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
	void checkAndRefresh_();

private:
	impl_::ContainerHelper	container_;

	std::string		path_;
};



} // namespace listloader
} // namespace

#endif

