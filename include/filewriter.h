#ifndef FILE_WRITTER_H_
#define FILE_WRITTER_H_

#include <fstream>
#include <cstdio>
#include <cassert>
#include <string_view>



/*
real	0m13.186s
user	0m11.201s
sys	0m1.726s
*/

struct FileWriterIOStream{
	constexpr static std::string_view name(){
		return "IOStream";
	}

	FileWriterIOStream() = default;
	FileWriterIOStream(std::string_view name, size_t size = 0) : FileWriterIOStream(name.data(), size){};

	FileWriterIOStream(const char *name, size_t = 0) : os(name, MODE){}

	bool write(const void *vdata, size_t size){
		const char *data = reinterpret_cast<const char *>(vdata);

		assert(data);

		return !! os.write(data, static_cast<std::streamsize>(size));
	}

	auto write(std::string_view s){
		return write(s.data(), s.size());
	}

	auto put(char c){
		return write(& c, 1);
	}

	void flush(){
		os.flush();
	}

	void close(){
		os.close();
	}

private:
	static constexpr auto MODE = std::ios::out | std::ios::binary | std::ios::trunc;

	std::ofstream os;
};



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
	FileWriterFOpen(std::string_view name, size_t size = 0) : FileWriterFOpen(name.data(), size){}

	FileWriterFOpen(const char *name, size_t = 0){
		f = fopen(name, "w");
	}

	~FileWriterFOpen(){
		if (f)
			fclose(f);
	}

	bool write(const void *vdata, size_t size){
		const char *data = reinterpret_cast<const char *>(vdata);

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



//using FileWriter = FileWriterFOpen;
using FileWriter = FileWriterIOStream;

#endif

