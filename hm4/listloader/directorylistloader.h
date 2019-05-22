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

		for (auto it = std::make_reverse_iterator(files.end()); it != std::make_reverse_iterator(files.begin()); ++it)
			if (files.isFile(*it)){
				container_.push_back(*it);

				if (DEBUG)
					printf("Consider %s\n", *it);
			}
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

