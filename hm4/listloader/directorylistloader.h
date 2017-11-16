#ifndef DIRECTORY_TABLE_LOADER_H_
#define DIRECTORY_TABLE_LOADER_H_

#include "baselistloader.h"


namespace hm4{
namespace listloader{


class DirectoryListLoader : public baselistloader_impl_::BaseListLoader{
public:
	using container_type	= std::vector<DiskList>;

public:
	DirectoryListLoader(std::string path, MMAPFile::Advice const advice = DEFAULT_ADVICE, DiskList::OpenMode const mode = DEFAULT_MODE) :
				BaseListLoader(advice, mode),
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

private:
	void refresh_();

private:
	std::string	path_;
};


} // namespace listloader
} // namespace


#endif

