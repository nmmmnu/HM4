#ifndef MYFS_H_
#define MYFS_H_

#include <string_view>
#include <cstdint>



bool fileExists(std::string_view name) noexcept;
bool fileUnlink(std::string_view name) noexcept;
uint64_t fileInode(std::string_view name) noexcept;
uint64_t checkFileInode(std::string_view filename) noexcept;


#endif

