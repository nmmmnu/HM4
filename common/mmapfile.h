#ifndef _MMAP_FILE_H
#define _MMAP_FILE_H

#include "stringref.h"

class MMAPFile{
public:
	const static int NORMAL		;
	const static int SEQUENTIAL	;
	const static int RANDOM		;

public:
	MMAPFile(int const madvise = NORMAL) :
				madvise_(madvise){}

	MMAPFile(MMAPFile &&other);

	~MMAPFile(){
		close();
	}

	bool open(const StringRef &filename);

	void close();

	operator bool() const{
		return mem_ != nullptr;
	}

	const void *mem() const{
		return mem_;
	}

	size_t size() const{
		return size_;
	}

private:
	bool open_(const StringRef &filename, int mode, int prot);

private:
	void	*mem_		= nullptr;
	size_t	size_		= 0;

	int	fd_;

	int	madvise_;

};

#endif


