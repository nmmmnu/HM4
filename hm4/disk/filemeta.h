#ifndef DISK_FILE_META_H_
#define DISK_FILE_META_H_

#include <fstream>

#include "myendian.h"

#include "filemetablob.h"
#include "stringref.h"


namespace hm4{
namespace disk{


class FileMeta{
private:
	FileMetaBlob	blob;

public:
	FileMeta(){
		clear();
	}

	bool open(const StringRef &filename){
		std::ifstream file_meta(filename, std::ios::in | std::ios::binary);

		return open(file_meta);
	}

	bool open(std::istream &file_meta);

	void clear(){
		// no need memset(), this is enought
		blob.version = 0;

		// this is nice safeguard
		blob.size = 0;
	}

public:
	operator bool() const{
		return blob.version;
	}

public:
	uint16_t version() const{
		return betoh<uint16_t>(blob.version);
	}

	bool sorted() const{
		return betoh<uint16_t>(blob.options) & FileMetaBlob::OPTIONS_SORTED;
	}

	bool aligned() const{
		return betoh<uint16_t>(blob.options) & FileMetaBlob::OPTIONS_ALIGNED;
	}

	uint64_t size() const{
		return betoh<uint64_t>(blob.size);
	}

public:
	void print() const;

private:
	template<typename UINT>
	static void printTime__(const char *descr, UINT const time);

	static void printBool__(const char *descr, bool const b);

private:
	bool openFail_(const StringRef &);
};


} // namespace
} // namespace hm4


#endif

