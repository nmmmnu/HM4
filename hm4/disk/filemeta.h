#ifndef DISK_FILE_META_H_
#define DISK_FILE_META_H_

#include <fstream>
#include <string_view>

#include "myendian.h"

#include "filemetablob.h"

namespace hm4{
namespace disk{


class FileMeta{
private:
	FileMetaBlob	blob;

public:
	FileMeta(){
		clear();
	}

	bool open(std::string_view const filename){
		std::ifstream file_meta(filename.data(), std::ios::in | std::ios::binary);

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

	auto size() const{
		return betoh<uint64_t>(blob.size);
	}

	auto created() const{
		return betoh<uint32_t>(blob.created);
	}

	auto createdMin() const{
		return betoh<uint64_t>(blob.createdMin);
	}

	auto createdMax() const{
		return betoh<uint64_t>(blob.createdMin);
	}

public:
	void print() const;

private:
	template<typename UINT>
	static void printTime__(const char *descr, UINT const time);

	static void printBool__(const char *descr, bool const b);

private:
	bool openFail_(std::string_view);
};


} // namespace
} // namespace hm4


#endif

