#ifndef _MMAP_FILE_H
#define _MMAP_FILE_H

#include <string_view>

class MMAPFile{
public:
	enum class Advice : char{
		NORMAL		,
		SEQUENTIAL	,
		RANDOM
	};

public:
	MMAPFile() = default;

	MMAPFile(MMAPFile &&other);

	~MMAPFile(){
		close();
	}

	bool open(std::string_view filename, Advice advice = Advice::NORMAL);

	void close();

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
	bool open_(std::string_view filename, int mode, int prot, int advice);

	static bool openFail__(int const fd);

	static int convertAdv__(Advice advice);

private:
	void	*mem_		= nullptr;
	size_t	size_		= 0;

	int	fd_;
};

#endif


