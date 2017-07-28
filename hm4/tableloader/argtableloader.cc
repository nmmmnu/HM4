#include "argtableloader.h"

namespace hm4{
namespace tableloader{


bool ArgTableLoader::refresh(){
	container_.clear();

	container_.reserve((container_type::size_type) argc_);

	// reverse for
	for(int i = argc_; i --> 0;)
		insert_(argv_[i]);

	return true;
}


} // namespace tableloader
} // namespace

