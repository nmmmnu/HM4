#ifndef _MMAP_FILE_H
#define _MMAP_FILE_H

#include <string_view>

namespace mmap_file_impl_{

	enum class Advice : char{
		NORMAL		,
		SEQUENTIAL	,
		RANDOM		,
		ALL
	};

} // namespace mmap_file_impl_



struct MMAPFileRO{
	using Advice = mmap_file_impl_::Advice;

	MMAPFileRO() = default;

	MMAPFileRO(MMAPFileRO &&other) :
			mem_		( std::move(other.mem_	)),
			size_		( std::move(other.size_	)){
		other.mem_ = nullptr;
	}

	~MMAPFileRO(){
		close();
	}

	bool open(std::string_view filename, Advice advice);

	bool openFD(int fd, size_t size, Advice advice);

	void close();

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
	void	*mem_		= nullptr;
	size_t	size_;
};

using MMAPFile = MMAPFileRO;



struct MMAPFileRW{
	using Advice = mmap_file_impl_::Advice;

	MMAPFileRW() = default;

	MMAPFileRW(MMAPFileRW &&other) :
			mem_		( std::move(other.mem_	)),
			size_		( std::move(other.size_	)){
		other.mem_ = nullptr;
	}

	~MMAPFileRW(){
		close();
	}

	bool open  (std::string_view filename, Advice advice);
	bool create(std::string_view filename, Advice advice, size_t size);

	bool openFD(int fd, size_t size, Advice advice);

	void close();

public:
	constexpr operator bool() const{
		return mem_ != nullptr;
	}

	constexpr const void *mem() const{
		return mem_;
	}

	constexpr void *mem(){
		return mem_;
	}

	constexpr size_t size() const{
		return size_;
	}

private:
	void	*mem_		= nullptr;
	size_t	size_;
};



#endif


