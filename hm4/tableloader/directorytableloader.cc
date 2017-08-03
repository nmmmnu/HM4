#include "directorytableloader.h"

#include "myglob.h"


namespace hm4{
namespace tableloader{


void DirectoryTableLoader::refresh_(){
	container_.clear();

	if (path_.empty())
		return;

	MyGlob files;
	if (files.open(path_) == false)
		return;

	container_.reserve(files.size());

	for (auto it = files.rbegin(); it != files.rend(); ++it)
		insert_(*it);
}


} // namespace tableloader
} // namespace

