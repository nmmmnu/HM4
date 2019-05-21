#ifndef DIRECTORY_TABLE_LOADER_H_
#define DIRECTORY_TABLE_LOADER_H_

#include "myglob.h"
#include "baselistloader.h"


namespace hm4{
namespace listloader{


template<class Container>
class DirectoryListLoader{
public:
	using DiskList = hm4::disk::DiskList;

	DirectoryListLoader(Container &container, std::string path, MMAPFile::Advice const advice = DEFAULT_ADVICE, DiskList::OpenMode const mode = DEFAULT_MODE) :
				inserter_(container, advice, mode),
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
	void refresh_(){
		inserter_.clear();

		if (path_.empty())
			return;

		MyGlob files;
		if (files.open(path_) == false)
			return;

		inserter_.reserve(files.size());

		for (auto it = files.rbegin(); it != files.rend(); ++it)
			inserter_(*it);

	}

private:
	impl_::Inserter<Container>	inserter_;

	std::string			path_;
};


} // namespace listloader
} // namespace


#endif

