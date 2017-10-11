#ifndef DISK_FILENAMES_H_
#define DISK_FILENAMES_H_

#include "stringref.h"

namespace hm4{
namespace disk{

namespace file_ext{
	constexpr const char *DOT_INDX		= ".indx";
	constexpr const char *DOT_DATA		= ".data";

	constexpr const char *DOT_BTREE_INDX	= ".tree";
	constexpr const char *DOT_BTREE_DATA	= ".keys";
} // namespace


inline std::string filenameMeta(std::string filename){
	return filename;
}


inline std::string filenameIndx(const StringRef &filename){
	return StringRef::concatenate( { filename, file_ext::DOT_INDX } );
}

inline std::string filenameData(const StringRef &filename){
	return StringRef::concatenate( { filename, file_ext::DOT_DATA } );
}


inline std::string filenameBTreeIndx(const StringRef &filename){
	return StringRef::concatenate( { filename, file_ext::DOT_BTREE_INDX } );
}

inline std::string filenameBTreeData(const StringRef &filename){
	return StringRef::concatenate( { filename, file_ext::DOT_BTREE_DATA } );
}


} // namespace diskfile
} // namespace

#endif

