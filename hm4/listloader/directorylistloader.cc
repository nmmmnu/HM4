#include "directorylistloader.h"

#include "myglob.h"
#include "logger.h"

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

	MyGlob files;
	if (files.open(path_) == false)
		return;

	container_.copy(std::begin(files), std::end(files), files.size());
}


} // namespace listloader
} // namespace

