#include "arglistloader.h"

namespace hm4{
namespace listloader{


bool ArgListLoader::refresh(){
	container_.clear();

	container_.reserve((container_type::size_type) argc_);

	// reverse for
	for(int i = argc_; i --> 0;)
		insert_(argv_[i]);

	return true;
}


} // namespace listloader
} // namespace

