#ifndef DISK_FILENAMES_H_
#define DISK_FILENAMES_H_

#include "stringref.h"

namespace hm4{
namespace disk{

namespace file_ext{
	constexpr const char *DOT_INDX		= ".indx";
	constexpr const char *DOT_DATA		= ".data";
	constexpr const char *DOT_LINE		= ".line";

	constexpr const char *DOT_BTREE_INDX	= ".tree";
	constexpr const char *DOT_BTREE_DATA	= ".keys";
} // namespace


inline auto filenameMeta(std::string filename){
	return filename;
}

inline auto filenameIndx(const StringRef &filename){
	return concatenate( filename, file_ext::DOT_INDX		);
}

inline auto filenameLine(const StringRef &filename){
	return concatenate( filename, file_ext::DOT_LINE		);
}

inline auto filenameData(const StringRef &filename){
	return concatenate( filename, file_ext::DOT_DATA		);
}


inline auto filenameBTreeIndx(const StringRef &filename){
	return concatenate( filename, file_ext::DOT_BTREE_INDX	);
}

inline auto filenameBTreeData(const StringRef &filename){
	return concatenate( filename, file_ext::DOT_BTREE_DATA	);
}

} // namespace diskfile
} // namespace

#endif

