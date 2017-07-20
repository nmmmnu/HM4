#include "filemeta.h"

#include <cstring>


namespace hm4{
namespace disk{


bool FileMeta::open(std::istream &file_meta){
	file_meta.read( (char *) &blob, sizeof(FileMetaBlob));

	if (! file_meta)
		return openFail_("Can not load file from disk.");

	if (file_meta.gcount() != sizeof(FileMetaBlob))
		return openFail_("File is different size.");

	// check the logo
	if (strncmp(blob.logo, FileMetaBlob::LOGO, strlen(FileMetaBlob::LOGO)) != 0)
		return openFail_("Logo wrong or missing");

	if (version() != FileMetaBlob::VERSION)
		return openFail_("Version wrong or missing");

	return true;
}

inline bool FileMeta::openFail_(const StringRef &){
	clear();

	return false;
}


} // namespace
} // namespace hm4


