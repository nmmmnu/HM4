#ifndef SINGLE_LIST_LOADER_H_
#define SINGLE_LIST_LOADER_H_

#include "disk/disklist.h"
#include "myfs.h"

namespace hm4{
namespace listloader{


class SingleListLoader{
public:
	using DiskList	= hm4::disk::DiskList;
	using List	= const DiskList;

public:
	template<typename UString>
	SingleListLoader(UString &&filename, DiskList::VMAllocator *allocator, DiskList::OpenMode const mode = DiskList::DEFAULT_MODE) :
				filename_(std::forward<UString>(filename)),
				allocator_(allocator),
				mode_(mode){
		open_();
	}

	template<typename UString>
	SingleListLoader(UString &&filename, DiskList::VMAllocator &allocator, DiskList::OpenMode const mode = DiskList::DEFAULT_MODE) :
		SingleListLoader(std::forward<UString>(filename), &allocator, mode){}

	template<typename UString>
	SingleListLoader(UString &&filename, DiskList::NoVMAllocator,          DiskList::OpenMode const mode = DiskList::DEFAULT_MODE) :
		SingleListLoader(std::forward<UString>(filename), nullptr,    mode){}

	void refresh(){
		auto const inode = fileInode(filename_);

		if (inode == list_.id())
			return;

		list_.close();
		list_.open(inode, filename_, allocator_, mode_);
	}

	// Command pattern
	bool command(){
		refresh();

		return true;
	}

	List const &getList() const{
		return list_;
	}

private:
	bool open_(){
		auto const inode = fileInode(filename_);

		if (inode == 0)
			return false;

		return list_.open(inode, filename_, allocator_, mode_);
	}

private:
	DiskList		list_;

	std::string		filename_;
	DiskList::VMAllocator	*allocator_;
	DiskList::OpenMode	mode_;
};


} // namespace listloader
} // namespace

#endif

