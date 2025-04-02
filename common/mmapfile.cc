#include "mmapfile.h"

#define _FILE_OFFSET_BITS 64

#include <sys/mman.h>	// mmap
#include <fcntl.h>	// open
#include <unistd.h>	// close, lseek



namespace{

	int convertAdv__(MMAPFile::Advice const advice){
		using Advice = MMAPFile::Advice;

		switch(advice){
		default:
		case Advice::NORMAL:		return MADV_NORMAL	;
		case Advice::SEQUENTIAL:	return MADV_SEQUENTIAL	;
		case Advice::RANDOM:		return MADV_RANDOM	;
		case Advice::ALL:		return MADV_WILLNEED	;
		}
	}

	bool openFail__(int const fd){
		::close(fd);

		return false;
	}

} // namespace



MMAPFile::MMAPFile(MMAPFile &&other) :
		mem_		( std::move(other.mem_	)),
		size_		( std::move(other.size_	)),
		rw_		( std::move(other.rw_	)){
	other.mem_ = nullptr;
}

bool MMAPFile::openRO(std::string_view filename, Advice const advice){
	close();

	int const mode = O_RDONLY;

	int const fd = ::open(filename.data(), mode);

	if (fd < 0)
		return false;

	off_t size2 = lseek(fd, 0, SEEK_END);

	if (size2 <= 0)
		return openFail__(fd);

	size_t const size = static_cast<size_t>(size2);

	return mmap_(false, size, fd, advice);
}

bool MMAPFile::openRW(std::string_view filename, Advice const advice){
	close();

	int const mode = O_RDWR;

	int const fd = ::open(filename.data(), mode);

	if (fd < 0)
		return false;

	off_t size2 = lseek(fd, 0, SEEK_END);

	if (size2 <= 0)
		return openFail__(fd);

	size_t const size = static_cast<size_t>(size2);

	return mmap_(true, size, fd, advice);
}

bool MMAPFile::create(std::string_view filename, Advice const advice, size_t size){
	close();

	int const mode = O_RDWR | O_CREAT | O_TRUNC;

	mode_t const chmod = 0600;

	int const fd = ::open(filename.data(), mode, chmod);

	if (fd < 0)
		return false;

	if (int const result = ftruncate(fd, static_cast<off_t>(size)); result < 0)
		return openFail__(fd);

	return mmap_(true, size, fd, advice);
}

bool MMAPFile::mmap_(bool rw, size_t size, int fd, Advice adviceC){
	int const prot = rw ?	PROT_READ | PROT_WRITE :
				PROT_READ;

	void *mem = mmap(nullptr, size, prot, MAP_SHARED, fd, /* offset */ 0);

	if (mem == MAP_FAILED)
		return openFail__(fd);

	int const advice = convertAdv__(adviceC);

	madvise(mem, size, advice);

	// the file no need to stay open
	::close(fd);

	mem_	= mem;
	size_	= size;
	rw_	= rw;

	return true;
}

void MMAPFile::close(){
	if (!mem_)
		return;

	munmap((void *) mem_, size_);

	mem_	= nullptr;
}

