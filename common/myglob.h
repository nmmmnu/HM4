#ifndef _MYGLOB_H
#define _MYGLOB_H

#include <string_view>

#include <glob.h>	// glob

class MyGlob final{
public:
	MyGlob(std::string_view path){
		open(path);
	}

	MyGlob() = default;

	MyGlob(const MyGlob &other) = delete;
	MyGlob& operator=(MyGlob other) = delete;

	MyGlob(MyGlob &&other) = default;
	MyGlob& operator=(MyGlob &&other) = default;

	~MyGlob() noexcept{
		close();
	}

public:
	bool open(std::string_view path) noexcept;
	void close() noexcept;

	bool isOpen() const noexcept{
		return isOpen_;
	}

public:
	size_t size() const noexcept{
		return globresults_.gl_pathc;
	}

	auto begin() const noexcept{
		return globresults_.gl_pathv;
	}

	auto end() const noexcept{
		return globresults_.gl_pathv + globresults_.gl_pathc;
	}

private:
	bool		isOpen_		= false;
	glob_t		globresults_;
};

#endif

