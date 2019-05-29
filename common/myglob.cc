#include "myglob.h"

#include <sys/stat.h>	// stat
#include <cassert>

bool MyGlob::isFile(const char *filename) noexcept{
	struct stat s;
	stat(filename, & s);

	if (s.st_mode & S_IFREG)	// file
		return true;

	if (s.st_mode & S_IFLNK)	// symlink
		return true;

	return false;
}

bool MyGlob::open(const StringRef &path) noexcept{
	assert(isOpen_ == false);

	int const result = glob(path.data(), 0, nullptr, & globresults_);

	if (result != 0)
		return false;

	if (result == GLOB_NOMATCH)
		return false;

	isOpen_ = true;

	return true;
}

void MyGlob::close() noexcept{
	if (isOpen_ == false)
		return;

	globfree(& globresults_);

	isOpen_ = false;
}

