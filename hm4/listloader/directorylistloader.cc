#include "directorylistloader.h"

#include "myglob.h"

namespace hm4{
namespace listloader{


void DirectoryListLoader::refresh_(){
	if (path_.empty())
		return;

	// guard against missing '*'
	if (path_.find('*') == std::string::npos)
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

