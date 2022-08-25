#include "myfs.h"

#include <unistd.h>	// access

bool fileExists(std::string_view const name){
	return access(name.data(), F_OK) == 0;
}

bool fileUnlink(std::string_view const name){
	return unlink(name.data()) == 0;
}

