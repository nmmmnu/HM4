#include "mmapfile.h"

#include <sys/mman.h>	// mmap
#include <fcntl.h>	// open
#include <unistd.h>	// close, lseek

MMAPFile::MMAPFile(MMAPFile &&other) :
		mem_		( std::move(other.mem_		)),
		size_		( std::move(other.size_		)),
		fd_		( std::move(other.fd_		)){
	other.mem_ = nullptr;
	other.size_ = 0;
}

bool MMAPFile::open(const StringRef &filename, const Advice advice){
	return open_(filename, O_RDONLY, PROT_READ, convertAdv__(advice));
}

bool MMAPFile::open_(const StringRef &filename, int const mode, int const prot, int const advice){
	close();

	int const fd = ::open(filename.data(), mode);

	if (fd < 0)
		return false;

	off_t size2 = lseek(fd, 0, SEEK_END);

	size_t size = size2 <= 0 ? 0 : size2;

	if (size == 0){
		::close(fd);
		return false;
	}

	/* const */ void *mem = mmap(nullptr, size, prot, MAP_SHARED, fd, /* offset */ 0);

	if (mem == MAP_FAILED){
		::close(fd);
		return false;
	}

	madvise(mem, size, advice);

	fd_ = fd;
	size_ = size;
	mem_ = mem;

	return true;
}

int MMAPFile::convertAdv__(const Advice advice){
	switch(advice){
	default:
	case Advice::NORMAL:		return MADV_NORMAL	;
	case Advice::SEQUENTIAL:	return MADV_SEQUENTIAL	;
	case Advice::RANDOM:		return MADV_RANDOM	;
	}


}

void MMAPFile::close(){
	if (mem_ == nullptr)
		return;

	munmap((void *) mem_, size_);
	::close(fd_);

	mem_ = nullptr;
	size_ = 0;
}

#if 0
bool MMAPFile::openRW(const StringRef &filename){
	return open_(filename, O_RDWR, PROT_READ | PROT_WRITE);
}

bool MMAPFile::createFile(const StringRef &filename, size_t const size){
	constexpr mode_t chmod = S_IRUSR | S_IWUSR;

	int const fd = ::open(filename.data(), O_RDWR | O_CREAT, chmod);

	if (fd < 0)
		return false;

	int const result = ftruncate(fd, size);

	::close(fd);

	return result == 0;
}
#endif

