#ifndef FILE_WRITTER_H_
#define FILE_WRITTER_H_

#include <cassert>
#include <string_view>

#include <cstdio>	// fopen

#include <cstring>	// memcpy
#include <fcntl.h>	// open
#include <unistd.h>	// close

#include "mybufferview.h"

/*
real	0m13.186s
user	0m11.201s
sys	0m1.726s
*/

// struct FileWriterIOStream;

/*
real	0m10.185s
user	0m8.564s
sys	0m1.392s
*/

struct FileWriterFOpen{
	constexpr static std::string_view name(){
		return "FOpen";
	}

	FileWriterFOpen() = default;

	FileWriterFOpen(std::string_view name, MyBuffer::ByteBufferView buffer) : FileWriterFOpen(name.data(), buffer){}

	FileWriterFOpen(const char *name, MyBuffer::ByteBufferView buffer){
		f = fopen(name, "w");

		if (buffer)
			setvbuf(f, static_cast<char *>(buffer.data()), _IOFBF, buffer.size());
	}

	~FileWriterFOpen(){
		if (f)
			fclose(f);
	}

	bool write(const void *vdata, size_t size){
		const char *data = static_cast<const char *>(vdata);

		assert(data);

		return f && fwrite(data, size, 1, f);
	}

	auto write(std::string_view s){
		return write(s.data(), s.size());
	}

	auto put(char c){
		return write(& c, 1);
	}

	void flush(){
		fflush(f);
	}

	void close(){
		if (f)
			fclose(f);

		f = nullptr;
	}

private:
	FILE *f = nullptr;
};



class FileWriterFD{
	int				fd_	= -1;
	size_t				pos_	=  0;
	MyBuffer::ByteBufferView	buffer_;

public:
	constexpr static std::string_view name(){
		return "FD";
	}

	FileWriterFD() = default;

	FileWriterFD(std::string_view name, MyBuffer::ByteBufferView buffer) : FileWriterFD(name.data(), buffer){}

	FileWriterFD(const char *name, MyBuffer::ByteBufferView buffer) : buffer_(buffer){
		int const mode = O_WRONLY; //O_RDWR;

		fd_ = ::open(name, mode);
	}

	~FileWriterFD(){
		if (fd_ < 0)
			return;

		flush();
		close();
	}

	void write(const void *data, size_t size) {
		if (fd_ < 0)
			return;

		assert(data);

		const char *p = static_cast<const char *>(data);
		      char *b = static_cast<      char *>(buffer_.data());

		while(size){
			size_t const space = buffer_.size() - size;

			if (size <= space) {
				memcpy(&b[pos_], p, size);
				pos_ += size;
			}else{
				memcpy(&b[pos_], p, space);
				pos_ += space;

				flush();

				p    += space;
				size -= space;
			}
		}
	}

	auto write(std::string_view s){
		return write(s.data(), s.size());
	}

	auto put(char c){
		return write(& c, 1);
	}

	void flush(){
		if (pos_ > 0) {
			::write(fd_, buffer_.data(), pos_);
			pos_ = 0;
		}
	}

	void close(){
		if (fd_ < 0)
			return;

		::close(fd_);
	}
};



using FileWriter = FileWriterFD;

#endif

