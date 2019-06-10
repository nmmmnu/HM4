#ifndef SINGLE_LIST_LOADER_H_
#define SINGLE_LIST_LOADER_H_

#include "disk/disklist.h"

namespace hm4{
namespace listloader{


class SingleListLoader{
public:
	using List = DiskList;

public:
	SingleListLoader(std::string filename, MMAPFile::Advice const advice = DiskList::DEFAULT_ADVICE, DiskList::OpenMode const mode = DiskList::DEFAULT_MODE) :
				filename_(std::move(filename)),
				advice_(advice),
				mode_(mode){
		open_();
	}

	bool refresh(){
		list_.close();

		return open_();
	}

	// Command pattern
	int command(int = 0){
		return refresh();
	}

	const auto &getList() const{
		return list_;
	}

private:
	int open_(){
		list_.open(filename_, advice_, mode_);
		return true;
	}

private:
	List			list_;

	std::string		filename_;
	MMAPFile::Advice	advice_;
	DiskList::OpenMode	mode_;
};


} // namespace listloader
} // namespace


#endif

