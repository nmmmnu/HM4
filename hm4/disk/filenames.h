#ifndef DISK_FILENAMES_H_
#define DISK_FILENAMES_H_

#include "mystring.h"

namespace hm4{
namespace disk{

namespace file_ext{
	constexpr const char *DOT_INDX		= ".indx";
	constexpr const char *DOT_DATA		= ".data";
	constexpr const char *DOT_LINE		= ".line";

	constexpr const char *DOT_BTREE_INDX	= ".tree";
	constexpr const char *DOT_BTREE_DATA	= ".keys";
} // namespace


inline auto filenameMeta(std::string_view const filename){
	return std::string{ filename };
}

inline auto filenameIndx(std::string_view const filename){
	return concatenate( filename, file_ext::DOT_INDX		);
}

inline auto filenameLine(std::string_view const filename){
	return concatenate( filename, file_ext::DOT_LINE		);
}

inline auto filenameData(std::string_view const filename){
	return concatenate( filename, file_ext::DOT_DATA		);
}


inline auto filenameBTreeIndx(std::string_view const filename){
	return concatenate( filename, file_ext::DOT_BTREE_INDX	);
}

inline auto filenameBTreeData(std::string_view const filename){
	return concatenate( filename, file_ext::DOT_BTREE_DATA	);
}

} // namespace diskfile
} // namespace

#endif

