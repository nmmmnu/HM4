#include "myfs.h"

#include <unistd.h>	// access
#include <sys/stat.h>	// stat

bool fileExists(std::string_view const name){
	return access(name.data(), F_OK) == 0;
}

bool fileUnlink(std::string_view const name){
	return unlink(name.data()) == 0;
}

uint64_t fileInode(std::string_view const name){
	struct stat buf;

	return stat(name.data(), &buf) ? 0 : buf.st_ino;
}

