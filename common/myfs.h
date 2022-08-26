#ifndef MYFS_H_
#define MYFS_H_

#include <string_view>
#include <cstdint>



bool fileExists(std::string_view const name);
bool fileUnlink(std::string_view const name);
uint64_t fileInode(std::string_view const name);



#endif

