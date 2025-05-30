#include "mmapfilesbo.h"

#include "logger.h"

#define _FILE_OFFSET_BITS 64

#include <fcntl.h>	// open
#include <unistd.h>	// close, lseek

bool MMAPFileSBO::open(VMAllocator &allocator, std::string_view const filename, MMAPFile::Advice advice){
	close();

	// -------------------------

	int const mode = O_RDONLY;

	int const fd = ::open(filename.data(), mode);

	if (fd < 0)
		return false;

	off_t size_off = lseek(fd, 0, SEEK_END);

	if (size_off < 0){
		::close(fd);
		return false;
	}

	size_t const size = static_cast<size_t>(size_off);

	// -------------------------

	if (size <= SmallSize){
		// go SBO

		if (char *data = MyAllocator::allocate<char>(allocator, size); data){

			logger<Logger::DEBUG>() << "Using SBO (small buffer optimization) for" << filename;

			// rewind...
			if (lseek(fd, 0, SEEK_SET) < 0){
				::close(fd);
				return false;
			}

			ssize_t const bytes = read(fd, data, size);

			// we need file closed anyway
			::close(fd);

			if (static_cast<size_t>(bytes) != size)
				return false;

			impl_.emplace<SBOFile>(data, size, allocator);

			return true;
		}
	}

	// go normal

	impl_.emplace<MMAPFile>();

	return std::get<MMAPFile>(impl_).openFD(fd, size, advice);
}

