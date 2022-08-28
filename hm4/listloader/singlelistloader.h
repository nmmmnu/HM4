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
	SingleListLoader(UString &&filename, MMAPFile::Advice const advice = DiskList::DEFAULT_ADVICE, DiskList::OpenMode const mode = DiskList::DEFAULT_MODE) :
				filename_(std::forward<UString>(filename)),
				advice_(advice),
				mode_(mode){
		open_();
	}

	void refresh(){
		auto const inode = fileInode(filename_);

		if (inode == list_.id())
			return;

		list_.close();
		list_.open(inode, filename_, advice_, mode_);
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

		return list_.open(inode, filename_, advice_, mode_);
	}

private:
	DiskList		list_;

	std::string		filename_;
	MMAPFile::Advice	advice_;
	DiskList::OpenMode	mode_;
};


} // namespace listloader
} // namespace

#endif

