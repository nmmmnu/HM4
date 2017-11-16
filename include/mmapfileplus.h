#ifndef MMAP_FILE_PLUS_H
#define MMAP_FILE_PLUS_H

#include "mmapfile.h"
#include "blobref.h"

class MMAPFilePlus{
public:
	MMAPFilePlus() = default;

	// no need d-tor,
	// MMAPFile-s will be closed automatically

public:
	bool open(const StringRef &filename, const MMAPFile::Advice advice = MMAPFile::Advice::NORMAL){
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

	template<class T>
	constexpr size_t sizeArray() const{
		return size() / sizeof(T);
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

