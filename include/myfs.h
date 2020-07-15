#ifndef MYFS_H_
#define MYFS_H_

#include <unistd.h>	// access

#include <string_view>


inline bool fileExists(std::string_view const name){
	return access(name.data(), F_OK) == 0;
}

inline bool fileUnlink(std::string_view const name){
	return unlink(name.data()) == 0;
}



#endif

