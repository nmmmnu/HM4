#ifndef _MMAP_FILE_SBO_H
#define _MMAP_FILE_SBO_H

#include "arenaallocator.h"

#include <string_view>

struct MMAPFileSBO{
	using Allocator = MyAllocator::ArenaAllocator;

public:
	struct SBOCapableResult{
		bool	ok	= false;
		int	fd	= 0;
		size_t	size	= 0;
	};

	static SBOCapableResult isSBOCapable(std::string_view filename);

public:
	MMAPFileSBO(Allocator &allocator) : allocator_(& allocator){}

	MMAPFileSBO(Allocator &allocator, std::string_view filename) : allocator_(& allocator){
		open(filename);
	}

	MMAPFileSBO(MMAPFileSBO &&other) :
			allocator_	( std::move(other.allocator_	)),
			mem_		( std::move(other.mem_		)),
			size_		( std::move(other.size_		)){
		other.mem_ = nullptr;
	}

	~MMAPFileSBO(){
		close();
	}

public:
	template<typename Advice>
	bool open(std::string_view filename, Advice){
		return open(filename);
	}

	template<typename Advice>
	bool openFD(int fd, size_t size, Advice){
		return openFD(fd, size);
	}

	constexpr void close(){
		MyAllocator::deallocate(*allocator_, mem_);
	}

public:
	bool open(std::string_view filename);

	bool openFD(int fd, size_t size);

public:
	constexpr operator bool() const{
		return mem_ != nullptr;
	}

	constexpr const void *mem() const{
		return mem_;
	}

	constexpr size_t size() const{
		return size_;
	}

private:
	Allocator	*allocator_;

	const void	*mem_		= nullptr;
	size_t		size_;
};

#endif


