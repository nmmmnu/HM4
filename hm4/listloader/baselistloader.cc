#include <sys/stat.h>	// stat

namespace{

	bool isFile(const char *filename) noexcept{
		struct stat s;
		stat(filename, & s);

		if (s.st_mode & S_IFREG)	// file
			return true;

		if (s.st_mode & S_IFLNK)	// symlink
			return true;

		return false;
	}

} // anonymous namespace

#include "baselistloader.h"

namespace hm4{
namespace listloader{
	namespace impl_{

		void ContainerHelper::push_back_(std::string_view filename){
			if (isFile(filename.data())){
				container_.emplace_back();
				container_.back().open(filename, advice_, mode_);
			}
		}

	} // namespace impl_
} // namespace listloader
} // namespace

