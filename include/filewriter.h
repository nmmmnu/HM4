#ifndef FILE_WRITTER_H_
#define FILE_WRITTER_H_

#include <fstream>
#include <cstdio>
#include <cassert>
#include <string_view>

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



using FileWriter = FileWriterFOpen;

#endif

