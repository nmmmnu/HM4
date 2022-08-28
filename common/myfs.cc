#include "myfs.h"

#include <unistd.h>	// access
#include <sys/stat.h>	// stat

bool fileExists(std::string_view const name) noexcept{
	return access(name.data(), F_OK) == 0;
}

bool fileUnlink(std::string_view const name) noexcept{
	return unlink(name.data()) == 0;
}

uint64_t fileInode(std::string_view const name) noexcept{
	struct stat buf;

	return stat(name.data(), &buf) ? 0 : buf.st_ino;
}

uint64_t checkFileInode(std::string_view const filename) noexcept{
	struct stat buf;
	stat(filename.data(), & buf);

	if (buf.st_mode & S_IFREG)	// file
		return buf.st_ino;

	if (buf.st_mode & S_IFLNK)	// symlink
		return buf.st_ino;

	return 0;
}

