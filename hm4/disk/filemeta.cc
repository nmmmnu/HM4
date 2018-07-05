#include "filemeta.h"

#include <cstring>

#include <inttypes.h>	// PRIu64

#include "mytime.h"

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

template<typename UINT>
void FileMeta::printTime__(const char *descr, UINT const time){
	printf("%-14s: %s\n",		descr,	time	? MyTime::toString(betoh(time)) : "n/a");
}

inline void FileMeta::printBool__(const char *descr, bool const b){
	printf("%-14s: %s\n",		descr,	b	? "Yes" : "No");
}

void FileMeta::print() const{
	printf("%-14s: %u\n",		"Version",	version()				);
	printf("%-14s: %" PRIu64 "\n",	"Records",	size()					); // PRIu64
	printf("%-14s: %" PRIu64 "\n",	"Tombstones",	betoh<uint64_t>(blob.tombstones)	); // PRIu64

	printBool__("Sorted",	sorted()	);
	printBool__("Aligned",	aligned()	);

	printTime__("Created",	blob.created	);

	printTime__("Created::MIN", blob.createdMin	);
	printTime__("Created::MAX", blob.createdMax	);
}


} // namespace
} // namespace hm4


