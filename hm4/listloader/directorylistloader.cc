#include "directorylistloader.h"

#include "myglob.h"
#include "logger.h"

#include <algorithm>

namespace hm4{
namespace listloader{

bool DirectoryListLoader::checkIfLoaderNeed(StringRef const &s){
	return std::find(std::begin(s), std::end(s), '*') != std::end(s);
}

void DirectoryListLoader::refresh_(){
	if (path_.empty())
		return;

	// guard against missing '*'
	if (checkIfLoaderNeed(path_) == false){
		log__("Refusing to open path without wildcard", path_);
		return;
	}

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

