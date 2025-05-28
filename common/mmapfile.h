#ifndef _MMAP_FILE_H
#define _MMAP_FILE_H

#include <string_view>

namespace mmap_file_impl_{

	enum class Advice : char{
		NORMAL		,
		SEQUENTIAL	,
		RANDOM
	};

} // namespace mmap_file_impl_



struct MMAPFileRO{
	using Advice = mmap_file_impl_::Advice;

	MMAPFileRO() = default;

	MMAPFileRO(std::string_view filename, Advice advice = Advice::NORMAL){
		open(filename, advice);
	}

	MMAPFileRO(MMAPFileRO &&other) :
			data_		( std::move(other.data_	)),
			size_		( std::move(other.size_	)){
		other.data_ = nullptr;
	}

	~MMAPFileRO(){
		close();
	}

public:
	bool open(std::string_view filename, Advice advice = Advice::NORMAL);

	bool openFD(int fd, size_t size, Advice advice = Advice::NORMAL);

	void close();

public:
	constexpr operator bool() const{
		return data_ != nullptr;
	}

	constexpr const void *data() const{
		return data_;
	}

	constexpr size_t size() const{
		return size_;
	}

private:
	void	*data_		= nullptr;
	size_t	size_;
};

using MMAPFile = MMAPFileRO;



struct MMAPFileRW{
	using Advice = mmap_file_impl_::Advice;

	MMAPFileRW() = default;

	MMAPFileRW(std::string_view filename, Advice advice = Advice::NORMAL){
		open(filename, advice);
	}

	MMAPFileRW(MMAPFileRW &&other) :
			data_		( std::move(other.data_	)),
			size_		( std::move(other.size_	)){
		other.data_ = nullptr;
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
		return data_;
	}

	constexpr const void *data() const{
		return data_;
	}

	constexpr void *data(){
		return data_;
	}

	constexpr size_t size() const{
		return size_;
	}

private:
	void	*data_		= nullptr;
	size_t	size_;
};



#endif


