#include "directorylistloader.h"

#include "myglob.h"

#include <algorithm>

namespace hm4{
namespace listloader{

namespace DirectoryListLoaderPath{
	bool checkPathWildcard(StringRef const &s){
		return std::find(std::begin(s), std::end(s), '*') != std::end(s);
	}

	bool checkPathWildcard(std::string const &s){
		return s.find('*') != std::string::npos;
	}
}

void DirectoryListLoader::refresh_(){
	if (path_.empty())
		return;

	// guard against missing '*'
	if (DirectoryListLoaderPath::checkPathWildcard(path_) == false)
		return;

	container_.clear();

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


} // namespace listloader
} // namespace

