#include "filemeta.h"

#include <cstring>

#include "mytime.h"

#define FMT_HEADER_ONLY
#include "fmt/printf.h"


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

inline bool FileMeta::openFail_(std::string_view){
	clear();

	return false;
}

void FileMeta::print() const{
	char buffer[MyTime::BUFFER_SIZE];

	const char *format = "{:<14}: {}\n";

	const char *time_format = MyTime::TIME_FORMAT_STANDARD;

	fmt::print(format,	"Version",	version()		);
	fmt::print(format,	"Records",	size()			);
	fmt::print(format,	"Tombstones",	betoh(blob.tombstones)	);

	fmt::print(format,	"Sorted",	sorted()  ? "Y" : "N"	);
	fmt::print(format,	"Aligned",	aligned() ? "Y" : "N"	);

	auto x = [&buffer, time_format](auto date){
		return date    ? MyTime::toString(betoh(date), time_format, buffer) : "n/a";
	};

	fmt::print(format,	"Created",	x(blob.created   	) );
	fmt::print(format,	"Created::MIN",	x(blob.createdMin	) );
	fmt::print(format,	"Created::MAX",	x(blob.createdMax	) );
}


} // namespace
} // namespace hm4


