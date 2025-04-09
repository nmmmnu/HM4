#ifndef DISK_FILENAMES_H_
#define DISK_FILENAMES_H_

#include "mystring.h"

namespace hm4{
namespace disk{

namespace file_ext{
	constexpr std::string_view DOT_TEMP		= ".temp";

	constexpr std::string_view DOT_INDX		= ".indx";
	constexpr std::string_view DOT_DATA		= ".data";
	constexpr std::string_view DOT_LINE		= ".line";
	constexpr std::string_view DOT_HASH		= ".hash";

	constexpr std::string_view DOT_BTREE_INDX	= ".tree";
	constexpr std::string_view DOT_BTREE_DATA	= ".keys";
} // namespace


inline std::string_view filenameMeta_string_view(std::string_view const filename){
	return filename;
}

inline auto filenameMetaTemp(std::string_view const filename){
	return concatenateString( filename, file_ext::DOT_TEMP	);
}

inline auto filenameIndx(std::string_view const filename){
	return concatenateString( filename, file_ext::DOT_INDX	);
}

inline auto filenameLine(std::string_view const filename){
	return concatenateString( filename, file_ext::DOT_LINE	);
}

inline auto filenameHash(std::string_view const filename){
	return concatenateString( filename, file_ext::DOT_HASH	);
}

inline auto filenameData(std::string_view const filename){
	return concatenateString( filename, file_ext::DOT_DATA	);
}


inline auto filenameBTreeIndx(std::string_view const filename){
	return concatenateString( filename, file_ext::DOT_BTREE_INDX	);
}

inline auto filenameBTreeData(std::string_view const filename){
	return concatenateString( filename, file_ext::DOT_BTREE_DATA	);
}

} // namespace diskfile
} // namespace

#endif

