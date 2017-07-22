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

	// order in the iterator, does not matter
	// but order in get, still very important
	for (auto it = files.rbegin(); it != files.rend(); ++it){
		const auto &filename = *it;

		container_.emplace_back();
                container_.back().open(filename);
	}

	return true;
}


} // namespace tableloader
} // namespace

