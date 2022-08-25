#ifndef MYFS_H_
#define MYFS_H_

#include <string_view>



bool fileExists(std::string_view const name);
bool fileUnlink(std::string_view const name);



#endif

