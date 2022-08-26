#include "directorylistloader.h"

#include "myglob.h"
#include "logger.h"

namespace hm4{
namespace listloader{



bool DirectoryListLoader::checkIfLoaderNeed(std::string_view const s){
	return std::find(std::begin(s), std::end(s), '*') != std::end(s);
}

void DirectoryListLoader::stop__(std::string_view const s){
	log__("Refusing to open path without wildcard", s);
	exit(1);
}

void DirectoryListLoader::refresh(){
	if (path_.empty())
		return;

	MyGlob files;
	if (files.open(path_))
		container_.copy(std::begin(files), std::end(files));
	else
		container_.copy();
}



} // namespace listloader
} // namespace

