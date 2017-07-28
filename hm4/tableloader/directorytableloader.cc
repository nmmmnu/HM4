#include "directorytableloader.h"

#include "myglob.h"


namespace hm4{
namespace tableloader{


bool DirectoryTableLoader::refresh(){
	container_.clear();

	MyGlob files;
	if (files.open(path_) == false)
		return true;

	container_.reserve(files.size());

	for (auto it = files.rbegin(); it != files.rend(); ++it)
		insert_(*it);

	return true;
}


} // namespace tableloader
} // namespace

