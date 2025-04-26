#include "mmapfile.h"

#define _FILE_OFFSET_BITS 64

#include <sys/mman.h>	// mmap
#include <fcntl.h>	// open
#include <unistd.h>	// close, lseek
#include <type_traits>



namespace{

	using Advice = mmap_file_impl_::Advice;

	int convertAdv__(Advice const advice){
		switch(advice){
		default:
		case Advice::NORMAL:		return MADV_NORMAL	;
		case Advice::SEQUENTIAL:	return MADV_SEQUENTIAL	;
		case Advice::RANDOM:		return MADV_RANDOM	;
	//	case Advice::ALL:		return MADV_WILLNEED	;
		}
	}



	struct FileResult{
		bool	ok	= false;
		int	fd	= 0;
		size_t	size	= 0;
	};

	FileResult file__(std::string_view filename, int mode){
		int const fd = ::open(filename.data(), mode);

		if (fd < 0)
			return {};

		off_t size2 = lseek(fd, 0, SEEK_END);

		if (size2 <= 0){
			::close(fd);
			return {};
		}

		return { true, fd, static_cast<size_t>(size2) };
	}



	struct PtrResult{
		bool	ok	= false;
		void	*mem	= 0;
		size_t	size	= 0;

		template<typename T>
		bool unpack(T *&mem, size_t &size) const{
			static_assert(
				std::is_same_v<
					std::remove_const_t<T>,
					void
				>,
				"Accepts void * only"
			);

			if (ok){
				mem	= this->mem;
				size	= this->size;
			}

			return ok;
		}
	};

	PtrResult mmap__(int fd, size_t size, int const prot, Advice adviceC){
		void *mem = mmap(nullptr, size, prot, MAP_SHARED, fd, /* offset */ 0);

		if (mem == MAP_FAILED){
			::close(fd);
			return {};
		}

		int const advice = convertAdv__(adviceC);

		madvise(mem, size, advice);

		// the file no need to stay open
		::close(fd);

		return { true, mem, size };
	}

} // namespace



bool MMAPFileRO::open(std::string_view filename, Advice const advice){
	close();

	int const mode = O_RDONLY;

	auto const r = file__(filename, mode);

	if (!r.ok)
		return false;

	int const prot = PROT_READ;

	auto const m = mmap__(r.fd, r.size, prot, advice);

	return m.unpack(mem_, size_);
}

bool MMAPFileRO::openFD(int fd, size_t size, Advice advice){
	close();

	int const prot = PROT_READ;

	auto const m = mmap__(fd, size, prot, advice);

	return m.unpack(mem_, size_);
}

void MMAPFileRO::close(){
	if (!mem_)
		return;

	munmap(mem_, size_);

	mem_ = nullptr;
}



bool MMAPFileRW::open(std::string_view filename, Advice const advice){
	close();

	int const mode = O_RDWR;

	auto const r = file__(filename, mode);

	if (!r.ok)
		return false;

	int const prot = PROT_READ | PROT_WRITE;

	auto const m = mmap__(r.fd, r.size, prot, advice);

	return m.unpack(mem_, size_);
}

bool MMAPFileRW::create(std::string_view filename, Advice const advice, size_t size){
	close();

	int const mode = O_RDWR | O_CREAT | O_TRUNC;

	mode_t const chmod = 0600;

	int const fd = ::open(filename.data(), mode, chmod);

	if (fd < 0)
		return false;

	if (int const result = ftruncate(fd, static_cast<off_t>(size)); result < 0){
		::close(fd);
		return false;
	}

	int const prot = PROT_READ | PROT_WRITE;

	auto const m = mmap__(fd, size, prot, advice);

	return m.unpack(mem_, size_);
}

void MMAPFileRW::close(){
	if (!mem_)
		return;

	munmap(mem_, size_);

	mem_ = nullptr;
}

