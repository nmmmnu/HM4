#ifndef DISK_FILENAMES_H_
#define DISK_FILENAMES_H_

#include "stringref.h"

namespace hm4{
namespace disk{


constexpr const char *DOT_INDX = ".indx";
constexpr const char *DOT_DATA = ".data";


inline std::string filenameMeta(const StringRef &filename){
	return filename;
}

inline std::string filenameIndx(const StringRef &filename){
	return StringRef::concatenate( { filename, DOT_INDX } );
}

inline std::string filenameData(const StringRef &filename){
	return StringRef::concatenate( { filename, DOT_DATA } );
}


} // namespace diskfile
} // namespace

#endif

