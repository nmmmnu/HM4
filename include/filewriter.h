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
build
real	8m29.119s
user	11m2.589s
sys	1m14.905s

merge
real	7m45.271s
user	1m44.751s
sys	1m12.147s

real	10m4.675s
user	1m50.903s
sys	1m39.171s
*/

// struct FileWriterIOStream;



/*
build
real	8m49.530s
user	10m39.708s
sys	1m19.855s

merge
real	9m3.387s
user	1m40.765s
sys	1m5.901s
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



/*
build
real	8m31.949s
user	9m58.846s
sys	1m18.215s

real	8m34.689s
user	10m3.363s
sys	1m21.935s

merge
real	9m2.521s
user	1m29.690s
sys	1m16.328s
*/

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
		int const mode = O_WRONLY | O_CREAT | O_TRUNC;

		fd_ = ::open(name, mode, 0644);
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

		if (size > buffer_.size()){
			flush();
			::write(fd_, data, size);
			return;
		}

		while(size > 0){
			size_t const space = buffer_.size() - pos_;

			if (space == 0){
				flush();
				continue;
			}

			size_t const toCopy = std::min(space, size);

			memcpy(&b[pos_], p, toCopy);

			pos_ += toCopy;
			p    += toCopy;
			size -= toCopy;
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

