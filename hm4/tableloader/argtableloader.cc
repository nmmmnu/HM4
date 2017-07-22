#include "argtableloader.h"

namespace hm4{
namespace tableloader{


bool ArgTableLoader::refresh_(){
	container_.clear();

	if (argc_ < 0)
		return true;

	container_.reserve((container_type::size_type) argc_);

	for(int i = 0; i < argc_; ++i){
		const char *filename = argv_[i];

		container_.emplace_back();
		container_.back().open(filename);
	}

	return true;
}


} // namespace tableloader
} // namespace

