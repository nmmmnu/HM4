#ifndef _MMAP_FILE_H
#define _MMAP_FILE_H

#include <string_view>

class MMAPFile{
public:
	enum class Advice : char{
		NORMAL		,
		SEQUENTIAL	,
		RANDOM		,
		ALL
	};

public:
	MMAPFile() = default;

	MMAPFile(MMAPFile &&other);

	~MMAPFile(){
		close();
	}

	bool open(std::string_view filename, Advice advice){
		return openRO(filename, advice);
	}

	bool openRO(std::string_view filename, Advice advice);
	bool openRW(std::string_view filename, Advice advice);
	bool create(std::string_view filename, Advice advice, size_t size);

	void close();

	constexpr operator bool() const{
		return mem_ != nullptr;
	}

	constexpr const void *mem() const{
		return memRO();
	}

	constexpr const void *memRO() const{
		return mem_;
	}

	constexpr void *memRW(){
		return rw_ ? mem_ : nullptr;
	}

	constexpr size_t size() const{
		return size_;
	}

	template<class T>
	constexpr size_t sizeArray() const{
		return size() / sizeof(T);
	}

private:
	bool mmap_(bool rw, size_t size, int fd, Advice advice);

private:
	void	*mem_		= nullptr;
	size_t	size_;
	bool	rw_;
};

#endif


