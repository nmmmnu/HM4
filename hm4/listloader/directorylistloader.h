#ifndef DIRECTORY_LIST_LOADER_H_
#define DIRECTORY_LIST_LOADER_H_

#include "myglob.h"
#include "baselistloader.h"

#include <algorithm>


namespace hm4{
namespace listloader{


template<class Container>
class DirectoryListLoader{
public:
	using DiskList = hm4::disk::DiskList;

	DirectoryListLoader(Container &container, std::string path, MMAPFile::Advice const advice = DiskList::DEFAULT_ADVICE, DiskList::OpenMode const mode = DiskList::DEFAULT_MODE) :
				container_(container, advice, mode),
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
		container_.clear();

		if (path_.empty())
			return;

		MyGlob files;
		if (files.open(path_) == false)
			return;

		container_.reserve(files.size());

		std::for_each(
			std::make_reverse_iterator(std::end  (files)),
			std::make_reverse_iterator(std::begin(files)),
			[&](const char *file){
				if (files.isFile(file))
					container_.push_back(file);
			}
		);
	}

private:
	constexpr static bool DEBUG = false;

private:
	using ContainerHelper = impl_::ContainerHelper<Container>;

	ContainerHelper	container_;

	std::string	path_;
};


} // namespace listloader
} // namespace


#endif

