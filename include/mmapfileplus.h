#ifndef MMAP_FILE_PLUS_H
#define MMAP_FILE_PLUS_H

#include "mmapfile.h"
#include "blobref.h"

struct MMAPFilePlus{
	bool open(std::string_view const filename, const MMAPFile::Advice advice = MMAPFile::Advice::NORMAL){
		file_.open(filename, advice);
		blob_ = { file_.mem(), file_.size() };

		return file_;
	}

	void close(){
		blob_.reset();
		file_.close();
	}

	constexpr operator bool() const{
		return file_;
	}

	constexpr size_t size() const{
		return file_.size();
	}

	constexpr const BlobRef &operator*() const{
		return blob_;
	}

	constexpr const BlobRef *operator->() const{
		return &blob_;
	}

private:
	MMAPFile	file_;
	BlobRef		blob_;
};

#endif

