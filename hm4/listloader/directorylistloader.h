#ifndef DIRECTORY_LIST_LOADER_H_
#define DIRECTORY_LIST_LOADER_H_

#include "baselistloader.h"

namespace hm4{
namespace listloader{



class DirectoryListLoader{
public:
	using DiskList	= hm4::disk::DiskList;
	using List 	= const impl_::ContainerHelper::CollectionList;

	using NoSlabAllocator = DiskList::NoVMAllocator;

public:
	template<typename UString>
	DirectoryListLoader(UString &&path, DiskList::VMAllocator *allocator, DiskList::OpenMode const mode = DiskList::DEFAULT_MODE) :
				container_(allocator, mode),
				path_(std::forward<UString>(path)){

		// guard against missing '*'
		checkAndRefresh_();
	}

	template<typename UString>
	DirectoryListLoader(UString &&path, DiskList::VMAllocator &allocator, DiskList::OpenMode const mode = DiskList::DEFAULT_MODE) :
				DirectoryListLoader(std::forward<UString>(path), &allocator, mode){}

	template<typename UString>
	DirectoryListLoader(UString &&path, DiskList::NoVMAllocator,          DiskList::OpenMode const mode = DiskList::DEFAULT_MODE) :
				DirectoryListLoader(std::forward<UString>(path), nullptr,     mode){}

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

